#include <sys/socket.h>
#include "sha1/sha1.h"
#include "peerComm.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <thread>  
#include <mutex>
#include <chrono>
#include <iomanip>

const int MAX_BLOCK_SIZE = 16384;

std::vector<unsigned char> decodeHashtoPureHex(std::string hashedString) {  //need to convert url encoded hash to pure hex
    std::vector<unsigned char> decodedHash;
    std::string subStringByte;
    for (int i = 1; i < hashedString.length(); i += 3) {
        unsigned char byte = static_cast<unsigned char>(std::stoi(hashedString.substr(i, 2), nullptr, 16));
        decodedHash.push_back(byte);
    };

    return decodedHash;
}

std::string decodeHashToBinary(std::string& hashString) {
    std::string hexStr;
    std::string binaryStr;
    for (int i = 1; i < hashString.length(); i+= 3) {
        hexStr = hashString.substr(i, 2);
        binaryStr += (static_cast<unsigned char>(std::strtol(hexStr.c_str(), nullptr,16)));
    }
    return binaryStr;
}

std::vector<unsigned char> createHandshake(torrentProperties& torrentContents) {    //creating the handshake to send to peer

    std::vector<unsigned char> handshake(68);
    std::string pstr = "BitTorrent protocol";

    handshake[0] = (unsigned char)0x13;
    memcpy(&handshake[1], pstr.c_str(), pstr.length());
    memset(&handshake[20], 0, 8);

    std::vector<unsigned char> infoHex = decodeHashtoPureHex(torrentContents.infoHash);
    std::vector<unsigned char> peerIDHex = decodeHashtoPureHex(torrentContents.peerID);

    memcpy(&handshake[28], infoHex.data(), 20);
    memcpy(&handshake[48], peerIDHex.data(), 20);

    return handshake;
}

peerStatus readPeerBitfield(int& clientSocket, peer& peer) {

    peerStatus peerStatusInfo;
    peerStatusInfo.peer = peer;

    std::vector<unsigned char>bitfieldBuffer;

    bitfieldBuffer.resize(5);   //resizing to hold msg length and msg type

    if((read(clientSocket,bitfieldBuffer.data(),bitfieldBuffer.size())) < 0) {
        std::cout << "error reading bitfield" << std::endl;
    }
    else {
        peerStatusInfo.bitFieldLength = static_cast<uint32_t>(bitfieldBuffer[0]) << 24 |
        static_cast<uint32_t>(bitfieldBuffer[1]) << 16 |
        static_cast<uint32_t>(bitfieldBuffer[2]) << 8 |
        static_cast<uint32_t>(bitfieldBuffer[3]);

        peerStatusInfo.bitFieldLength -= 1; //to adjust for msg type byte

        bitfieldBuffer.resize(peerStatusInfo.bitFieldLength + 5);   //resizing to grab bitfield, (+ 5 in case peer sends unchoke as well to avoid duplication)

        read(clientSocket,bitfieldBuffer.data(),bitfieldBuffer.size());
        peerStatusInfo.bitfield = std::vector<unsigned char>(bitfieldBuffer.begin(),bitfieldBuffer.end()-5);

        std::cout << "peer: " << peerStatusInfo.peer.ip << std::endl;
        std::cout << "bitfield length: " << peerStatusInfo.bitFieldLength << std::endl;
        std::cout << "bitfield: ";
        for (size_t i = 0; i < peerStatusInfo.bitFieldLength; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(peerStatusInfo.bitfield[i]) << " ";
        }
        std::cout << std::dec << std::endl; 
    }
    return peerStatusInfo;
}

int waitForUnchoke(int& cilentSocket, peerStatus& peerStatusInfo) {
    std::vector<unsigned char> unchokeBuffer(5);

    ssize_t unchokeResponse;
    int counter = 0;
    while(counter < 5) {
        unchokeResponse = read(cilentSocket, unchokeBuffer.data(), unchokeBuffer.size());
        if (unchokeResponse == 0) {
            counter++;
            sleep(2);
        }
        else if (unchokeResponse == -1) {
            std::cout << "Error getting unchoked" << std::endl;
            return -1;
        }
        else {
            if(unchokeBuffer[4] == 1) {
                return 0;
            }    //got unchoked
        }
    }
    return -1;
}

peerStatus parsePeerMessage(std::vector<unsigned char>& response_buffer, peer& peer) {

    peerStatus peerStatusInfo;
    peerStatusInfo.peer = peer;
    peerStatusInfo.bitFieldLength = static_cast<uint32_t>(response_buffer[68]) << 24 |
    static_cast<uint32_t>(response_buffer[69]) << 16 |
    static_cast<uint32_t>(response_buffer[70]) << 8 |
    static_cast<uint32_t>(response_buffer[71]);

    std::vector<unsigned char> bitfield(static_cast<int>(peerStatusInfo.bitFieldLength));

    for (int i = 0; i < peerStatusInfo.bitFieldLength; i++) {
         bitfield[i] = response_buffer[73+i];
    }
    peerStatusInfo.bitfield = bitfield;

    std::cout << "peer: " << peerStatusInfo.peer.ip << std::endl;
    std::cout << "bitfield length: " << peerStatusInfo.bitFieldLength << std::endl;
    std::cout << "bitfield: ";
    for (size_t i = 0; i < peerStatusInfo.bitFieldLength; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(response_buffer[i+72]) << " ";
    }
    std::cout << std::dec << std::endl; 


    
    return peerStatusInfo;

}
bool peerHasPiece(std::vector<unsigned char>& bitfield, int& index) {
    int byteIndex = index / 8;
    int offset = index % 8;
    return bitfield[index] >> ((7-offset) & 1) != 0;
}

std::map<int, std::string>PeerPieceInfo(std::vector<unsigned char>& bitfield, int& totalPieces) {
    std::map<int, std::string> peerPieceInfo;

    for (int i = 0; i < totalPieces; i++) {
        if (peerHasPiece(bitfield, i)) {
            peerPieceInfo[i] = "has piece";
        }
        else {
            peerPieceInfo[i] = "Does not have piece";
        }
    };
    return peerPieceInfo;
};

int connectToPeer(peer& peer, torrentProperties& torrentContents) {


    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);             //creating socket
    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    fcntl(clientSocket, F_SETFL, O_NONBLOCK);

    struct timeval timeout;
    fd_set fd;
    timeval tv;
    FD_ZERO(&fd);
    FD_SET(clientSocket, &fd);

    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    sockaddr_in peerAddress;
    peerAddress.sin_family = AF_INET;
    peerAddress.sin_port = htons(peer.port);
    inet_pton(AF_INET, peer.ip.c_str(), &peerAddress.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&peerAddress, sizeof(peerAddress)) == -1 && errno != EINPROGRESS) {
        std::cerr << "Connection error " << peer.ip << "-> " << strerror(errno) << std::endl;
        return -1;
    }

    else if (select(clientSocket + 1, nullptr, &fd, nullptr, &timeout) <= 0) {
        std::cerr << "Connection timed out to: " << peer.ip << std::endl;
        return -1; 
    }

    fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_SETFL) & ~O_NONBLOCK);

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return clientSocket;
};
    
int sendPeerHandshake(int& clientSocket, std::vector<unsigned char> handshake, peer& peer) {
        std::vector<unsigned char> handshakeResponseBuffer(68);

    if(send(clientSocket, handshake.data(), 68, 0) == -1) {
        std::cout << "Could not send the handshake to: " << peer.ip << ":" << peer.port << std::endl;
        return -1;
    }
    while (true) {
        sleep(5);   //waiting for handshake back
        ssize_t bytesRead = read(clientSocket, handshakeResponseBuffer.data(), handshakeResponseBuffer.size());
        if (bytesRead == -1) {
             std::cout << "error receiving handshake from: " << peer.ip << std::endl;          //reads response from handshake
             return -1;
        }
        else if (bytesRead == 0) {
            std::cout << "no response for handshake from: " << peer.ip << std::endl;
            return -1;
        }
        break;
    }


    for (int i = 0; i < 20; i++) {
        if (handshakeResponseBuffer[i+28] != handshake[i+28]) {
            std::cout << "handshakes don't match!" << std::endl;    //checking if handshakes match
            return -1;
        }
    }
    std::cout << "successful handshake with: " << peer.ip << ":" << peer.port << std::endl;
    return 0;
}

int sendInterestedMsg(int& clientSocket) {
    std::vector<unsigned char> interestedMsg(5);
    std::vector<unsigned char> responseMsg(5);
    ssize_t peerResponse;
    interestedMsg[0] = 0;
    interestedMsg[1] = 0;
    interestedMsg[2] = 0;
    interestedMsg[3] = 1;
    
    interestedMsg[4] = 2;

    if(send(clientSocket, interestedMsg.data(), interestedMsg.size(), 0) == -1) {
        perror("Could not send interested message");
        return -1;
    }
    return 0;
}

std::vector<unsigned char> createRequestMsg(int piece, int beginIndex, int blockLength) {
    std::vector<unsigned char> requestMsg(17,0);

    requestMsg[0] = 0;
    requestMsg[1] = 0;  //setting the length of the msg
    requestMsg[2] = 0;
    requestMsg[3] = 13;

    requestMsg[4] = 6;

    requestMsg[5] = (piece >> 24) & 0xFF;
    requestMsg[6] = (piece >> 16) & 0xFF;  //setting the piece index of the msg
    requestMsg[7] = (piece >> 8) & 0xFF;
    requestMsg[8] = (piece) & 0xFF;

    requestMsg[9] = (beginIndex >> 24) & 0xFF;
    requestMsg[10] = (beginIndex >> 16) & 0xFF;  //setting the starting index of the block
    requestMsg[11] = (beginIndex >> 8) & 0xFF;
    requestMsg[12] = (beginIndex) & 0xFF;

    requestMsg[13] = (blockLength >> 24) & 0xFF;
    requestMsg[14] = (blockLength >> 16) & 0xFF;  //setting the length of the block
    requestMsg[15] = (blockLength >> 8) & 0xFF;
    requestMsg[16] = (blockLength) & 0xFF;

    return requestMsg;

}

std::mutex mtx;

void downloadAvailablePieces(std::map<int, std::string>& peerPieceInfo, int& clientSocket, torrentProperties& torrentContent) {
    const ssize_t expectedMsgSize = 16397;
    ssize_t bytesRead;
    std::vector<unsigned char> pieceBlockBuffer(expectedMsgSize, 255);
    std::vector<unsigned char> fullPieceData(torrentContent.pieceLength, 255);
    std::vector<unsigned char> requestMsg;
    int pieceIndexForDownload = 0;

    while(torrentContent.piecesToBeDownloaded.size() > 0) {
        mtx.lock();
        if(peerPieceInfo[torrentContent.piecesToBeDownloaded[torrentContent.piecesToBeDownloaded.size()-1]] == "has piece") {
            pieceIndexForDownload = torrentContent.piecesToBeDownloaded[torrentContent.piecesToBeDownloaded.size()-1];
            torrentContent.piecesToBeDownloaded.pop_back();
        }
        else {
            pieceIndexForDownload = -1;
        }
        mtx.unlock();

        if(pieceIndexForDownload != -1) {

            for (int j = 0; j < (torrentContent.pieceLength/MAX_BLOCK_SIZE); j++) {
                std::fill(pieceBlockBuffer.begin(), pieceBlockBuffer.end(), 255);
                //std::cout << "loop size: " << torrentContent.pieceLength/MAX_BLOCK_SIZE << std::endl;
                //std::cout << "MAX_BLOCK_SIZE: " << MAX_BLOCK_SIZE << std::endl;
                requestMsg = createRequestMsg(pieceIndexForDownload, j*MAX_BLOCK_SIZE, MAX_BLOCK_SIZE);

                if(send(clientSocket, requestMsg.data(), requestMsg.size(), 0) == -1) {
                    perror("send failed");
                    std::cout << "send for block: " << j << " for piece: " << pieceIndexForDownload << " couldn't send" << std::endl;
                    return;
                } 
                else {
                    bytesRead = 0;
                    while (bytesRead < expectedMsgSize) {
                        ssize_t peerBlockRepsonse = read(clientSocket, pieceBlockBuffer.data()+bytesRead, expectedMsgSize-bytesRead);

                        if (peerBlockRepsonse <= 0) {
                            //std::cout << "error grabbing piece" << std::endl;
                            break;
                        }

                        bytesRead += peerBlockRepsonse;
                    }

                    if (bytesRead != expectedMsgSize) {
                        //std::cout << "Unexpected block size..." << std::endl;
                        mtx.lock();
                        torrentContent.piecesToBeDownloaded.push_back(pieceIndexForDownload);
                        mtx.unlock();
                        break;
                    }
                        
                        //std::cout << "msg size: " << static_cast<int>(bytesRead) << std::endl;
                        //std::cout << "4th index: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pieceBlockBuffer[4]) << std::endl;
                        //std::cout << std::dec << std::endl;
                        //std::cout << "Payload: " << std::endl;
                        //sleep(1);
                        //for (size_t i = 0; i < expectedMsgSize; i++) {
                            //std::cout << std::hex << std::setw(2) << std::setfill('0')
                                    //<< static_cast<int>(pieceBlockBuffer[i]) << " ";
                        //}
                        //std::cout << std::dec << std::endl; // Reset to decimal output



                        if (static_cast<int>(pieceBlockBuffer[4]) == 7) {
                            //std::cout << "successfully downloaded block: " << j << " for piece: " << piecesPeerHas[i] << std::endl;
                            std::copy(pieceBlockBuffer.begin()+13, pieceBlockBuffer.end(),fullPieceData.begin()+(j*MAX_BLOCK_SIZE));
                            //std::cout << "Downloaded: " << std::setprecision(2) << (static_cast<float>(torrentContent.piecesDownloadedOrInProgress.size()) / static_cast<float>(torrentContent.numOfPieces)) << "%" << std::endl;
                        }
                        else {
                            std::cout << "received wrong response message! for block: " << j << std::endl;
                            mtx.lock();
                            torrentContent.piecesToBeDownloaded.push_back(pieceIndexForDownload);
                            mtx.unlock();
                            break;
                        }
                }
            }
            std::string downloadedPieceHash = sha1Hash(std::string(fullPieceData.begin(), fullPieceData.end()));
            std::string downloadedPieceHashBinary = decodeHashToBinary(downloadedPieceHash);
            //std::cout << "torrent file hash: " << torrentContent.pieceHashes[i] << std::endl;
            //std::cout << "calcualted hash: " << downloadedPieceHashBinary << std::endl;
            if (downloadedPieceHashBinary != torrentContent.pieceHashes[pieceIndexForDownload]) {
                std::cout << "piece integrity bad!!" << std::endl;
                mtx.lock();
                torrentContent.piecesToBeDownloaded.push_back(pieceIndexForDownload);
                mtx.unlock();
            
            }
            else {
                std::cout << "piece integrity good!!" << std::endl;
                torrentContent.downloadedPieces.push_back(pieceIndexForDownload);
                //std::cout << "successfully downloaded piece: " << i << std::endl;
                std::cout << "downloaded set: (";
                for (const int &pieceIndex : torrentContent.downloadedPieces) {
                    std::cout << pieceIndex << " ";
                }
                std::cout << ")" << std::endl;
                torrentContent.fileBuiltPieces[pieceIndexForDownload] = fullPieceData;
                std::cout << "Downloaded: " << std::setprecision(2) << (static_cast<float>(torrentContent.downloadedPieces.size()) / static_cast<float>(torrentContent.numOfPieces)) << "%" << std::endl;

            }
            std::fill(fullPieceData.begin(), fullPieceData.end(), 255);
        }
        }



}


void communicateWithPeers(announceProperties& announceContent, torrentProperties& torrentContents, peer& peer) {
    peerStatus peerMsg;
        //sleep(5);
        //this where you put thread!!! (instead of loop)
        int clientSocket = connectToPeer(peer, torrentContents);
        if (clientSocket != -1) {
            std::vector<unsigned char> handshake = createHandshake(torrentContents);

            if (sendPeerHandshake(clientSocket, handshake, peer) != -1) {
                peerStatus peerStatusInfo = readPeerBitfield(clientSocket, peer);
                if (peerStatusInfo.bitFieldLength > 0) {
                    if(sendInterestedMsg(clientSocket) != -1) {
                        if(waitForUnchoke(clientSocket, peerStatusInfo) != -1) {
                        std::cout << "not choked! yay!" << std::endl;
                        std::map<int, std::string> peerPieces = PeerPieceInfo(peerStatusInfo.bitfield, torrentContents.numOfPieces);
                        downloadAvailablePieces(peerPieces, clientSocket, torrentContents);
                        }
                    }
                }
            }
        }
        close(clientSocket);
        return;


}