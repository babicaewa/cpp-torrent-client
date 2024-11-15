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
#include <signal.h>
#include <algorithm>

const int MAX_BLOCK_SIZE = 16384;
const int MAX_SIMULTANEOUS_REQUESTS = 16;
const ssize_t EXPECTED_MSG_SIZE = MAX_BLOCK_SIZE + 13;
const int MAX_WAIT_TIME = 20;

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
    peerStatusInfo.choked = true;

    std::vector<unsigned char>bitfieldBuffer;

    bitfieldBuffer.resize(5);   //resizing to hold msg length and msg type

    if((read(clientSocket,bitfieldBuffer.data(),bitfieldBuffer.size())) <= 0) {
        std::cout << "error reading bitfield" << std::endl;
    }
    else {
        if(bitfieldBuffer[4] == 5) {
        peerStatusInfo.bitFieldLength = static_cast<uint32_t>(bitfieldBuffer[0]) << 24 |
        static_cast<uint32_t>(bitfieldBuffer[1]) << 16 |
        static_cast<uint32_t>(bitfieldBuffer[2]) << 8 |
        static_cast<uint32_t>(bitfieldBuffer[3]);

        peerStatusInfo.bitFieldLength -= 1; //to adjust for msg type byte

        if (peerStatusInfo.bitFieldLength == 0) {
            return peerStatusInfo;
        }

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
        }
    return peerStatusInfo;
}

int waitForUnchoke(int& cilentSocket, peerStatus& peerStatusInfo) {
    std::vector<unsigned char> unchokeBuffer(5);

    ssize_t unchokeResponse;
    int counter = 0;
    while(counter < 10) {
        unchokeResponse = read(cilentSocket, unchokeBuffer.data(), unchokeBuffer.size());
        //std::cout << "unchokeResponse: " << unchokeResponse << ", counter: " << counter << std::endl;

        if (unchokeResponse == 0) {
            counter++;
            sleep(1);
        }
        else if (unchokeResponse == -1) {
            std::cout << "Error getting unchoked" << std::endl;
            return -1;
        }
        else {
            if(unchokeBuffer[4] == 1) {
                peerStatusInfo.choked = false;
                return 0;
            }    //got unchoked
        }
    }
    return -1;
}

bool peerHasPiece(std::vector<unsigned char>& bitfield, int& index) {
    int byteIndex = index / 8;
    int offset = index % 8;
    return bitfield[byteIndex] >> ((7-offset) & 1) != 0;
}

/*
std::map<int, std::string>PeerPieceInfo(std::vector<unsigned char>& bitfield, int& totalPieces) {
    std::map<int, std::string> peerPieceInfoMap;

    for (int i = 0; i < totalPieces; i++) {
        if (peerHasPiece(bitfield, i)) {
            peerPieceInfoMap[i] = "has piece";
        }
        else {
            peerPieceInfoMap[i] = "Does not have piece";
        }
    };
    return peerPieceInfoMap;
};

*/

int connectToPeer(peer& peer, torrentProperties& torrentContents) {


    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);             //creating socket
    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    fcntl(clientSocket, F_SETFL, O_NONBLOCK);
    signal(SIGPIPE, SIG_IGN);   //to ignore broken pipe program termination

    struct timeval timeout;
    fd_set fd;
    timeval tv;
    FD_ZERO(&fd);
    FD_SET(clientSocket, &fd);

    timeout.tv_sec = 5;
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
        std::cout << "Could not send the handshake to: " << peer.ip << ":" << peer.port << "->" << strerror(errno) << std::endl;
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

int sendHaveMsg(int& clientSocket, int&index) {
    std::vector<unsigned char> haveMsg(9,0);
    haveMsg[3] = 5;
    haveMsg[4] = 4;
    haveMsg[5] = (index >> 24) & 0xFF;
    haveMsg[6] = (index >> 16) & 0xFF;
    haveMsg[7] = (index >> 8) & 0xFF;
    haveMsg[8] = index & 0xFF;


    if(send(clientSocket, haveMsg.data(), haveMsg.size(), 0) == -1) {
        perror("Could not send have message");
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

int sendRequestMsgs(int& clientSocket, int& pieceIndexForDownload, int& startIndex, int endIndex) {
    std::vector<unsigned char> requestMsg;
    for (int i = startIndex; i < endIndex; i++) {
       requestMsg = createRequestMsg(pieceIndexForDownload, i*MAX_BLOCK_SIZE, MAX_BLOCK_SIZE); 
       ssize_t sentMsg = send(clientSocket, requestMsg.data(), requestMsg.size(), 0);
        if(sentMsg == -1) {
            perror("send failed");
            std::cout << "send for block: " << i << " for piece: " << pieceIndexForDownload << " couldn't send" << std::endl;
            return -1;
            } 
        else if (sentMsg == 0) {
            std::cout << "socket closed???" << std::endl;
            return -1;
        }
    }
    return 0;
}

int downloadRequestedPieces(int& clientSocket, int& pieceIndexForDownload, int& startIndex, int endIndex, std::vector<unsigned char>& fullPieceData, torrentProperties& torrentContent, std::mutex& queueMutex, peerStatus& peerStatusInfo) {
    std::vector<unsigned char> pieceBlockBuffer(EXPECTED_MSG_SIZE, 0);  //does not account for partial piece!!!!
    for (int i = startIndex; i < endIndex; i++) {
       auto startTime = std::chrono::steady_clock::now();

        ssize_t bytesRead = 0;
        while (bytesRead < EXPECTED_MSG_SIZE) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
            if (elapsedSeconds > MAX_WAIT_TIME) {
                return -1;
            }
            ssize_t peerBlockRepsonse = read(clientSocket, pieceBlockBuffer.data()+bytesRead, EXPECTED_MSG_SIZE-bytesRead);
            if (peerBlockRepsonse < 0) {
                return -1;
            }
            bytesRead += peerBlockRepsonse;
        }
        if (bytesRead != EXPECTED_MSG_SIZE) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                torrentContent.piecesToBeDownloadedSet.insert(pieceIndexForDownload);
                torrentContent.piecesQueue.erase(pieceIndexForDownload);
            }
            return -1;
        }

        if (static_cast<int>(pieceBlockBuffer[4]) == 7) {
            int pieceOffset = static_cast<uint32_t>(pieceBlockBuffer[9]) << 24 |
                            static_cast<uint32_t>(pieceBlockBuffer[10]) << 16 |
                            static_cast<uint32_t>(pieceBlockBuffer[11]) << 8 |
                            static_cast<uint32_t>(pieceBlockBuffer[12]);             
            for (int j = 0; j < (pieceBlockBuffer.size()-13); j++) {
                    if ((pieceOffset + j) >= fullPieceData.size()) {
        std::cerr << "Error: Attempting to access out of bounds in fullPieceData at index "<< (pieceOffset + j) << std::endl;
        break;
    }
                fullPieceData[pieceOffset+j] = (pieceBlockBuffer[j+13]);
            }
            //std::copy(pieceBlockBuffer.begin()+13, pieceBlockBuffer.end(),fullPieceData.begin()+pieceOffset); // +pieceOffset faster, (i*MAX_BLOCK_SIZE) sloeer, but causes seg fault whatever)
            std::fill(pieceBlockBuffer.begin(), pieceBlockBuffer.end(), 0);
            }
            else if (static_cast<int>(pieceBlockBuffer[4]) == 0)  {
                //std::cout << "received wrong response message! for block: " << j << std::endl;
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    torrentContent.piecesToBeDownloadedSet.insert(pieceIndexForDownload);
                    torrentContent.piecesQueue.erase(pieceIndexForDownload);
                }
                peerStatusInfo.choked = true;
                if (waitForUnchoke(clientSocket, peerStatusInfo) == -1) {
                    return -1;
                };

            }

    }
    return 0;
}



int downloadAvailablePieces(int& clientSocket, torrentProperties& torrentContent, std::mutex& queueMutex, peerStatus& peerStatusInfo, peer& peer) {
    ssize_t bytesRead;
    std::vector<unsigned char> fullPieceData(torrentContent.pieceLength, 0);
    std::vector<unsigned char> requestMsg;
    int pieceIndexForDownload = 0;
    int blockRequestIndex;
    int totalBlocksNeeded = torrentContent.pieceLength/MAX_BLOCK_SIZE;

    while(torrentContent.fileBuiltPieces.size() < torrentContent.numOfPieces) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            pieceIndexForDownload = -1;
            //pieceIndexForDownload = *torrentContent.piecesToBeDownloadedSet.begin();
            auto queueIterator = torrentContent.piecesToBeDownloadedSet.begin();
            while (queueIterator != torrentContent.piecesToBeDownloadedSet.end()) {
                int piece = *queueIterator;
                if (peerHasPiece(peerStatusInfo.bitfield, piece) && torrentContent.fileBuiltPieces.find(piece) == torrentContent.fileBuiltPieces.end() 
                && torrentContent.piecesQueue.find(piece) == torrentContent.piecesQueue.end()
                && peer.problemDownloadingSet.find(piece) == peer.problemDownloadingSet.end()) {
                    //std::cout << "peer has piece: " << piece << std::endl;      //fix this!!!
                    pieceIndexForDownload = piece;
                    torrentContent.piecesToBeDownloadedSet.erase(queueIterator);
                    torrentContent.piecesQueue.insert(piece);
                    break;
                }
                ++queueIterator;
            }
        }
        if(pieceIndexForDownload != -1) {
            blockRequestIndex = 0;
            while(blockRequestIndex < totalBlocksNeeded) {
                int endBlockIndex = std::min(blockRequestIndex+MAX_SIMULTANEOUS_REQUESTS, totalBlocksNeeded);
                if(sendRequestMsgs(clientSocket,pieceIndexForDownload,blockRequestIndex,endBlockIndex) == 0) {
                    if (downloadRequestedPieces(clientSocket, pieceIndexForDownload, blockRequestIndex, endBlockIndex, fullPieceData, torrentContent, queueMutex, peerStatusInfo) == -1) {
                        std::cout << "failed to download the requested pieces for piece: " << pieceIndexForDownload << std::endl;
                        //sleep(1);
                        //std::cout << "size of array of pieces needed left: " << torrentContent.piecesToBeDownloadedSet.size() << std::endl;
                        break;
                    };
                } else {
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        torrentContent.piecesToBeDownloadedSet.insert(pieceIndexForDownload);
                        torrentContent.piecesQueue.erase(pieceIndexForDownload);
                    
                    }
                    return -1;
                }
                blockRequestIndex += MAX_SIMULTANEOUS_REQUESTS;
            }
            std::string downloadedPieceHash = sha1Hash(std::string(fullPieceData.begin(), fullPieceData.end()));
            std::string downloadedPieceHashBinary = decodeHashToBinary(downloadedPieceHash);
            //std::cout << "torrent file hash: " << torrentContent.pieceHashes[i] << std::endl;
            //std::cout << "calcualted hash: " << downloadedPieceHashBinary << std::endl;
            if (downloadedPieceHashBinary != torrentContent.pieceHashes[pieceIndexForDownload]) {
                std::cout << "piece integrity bad!! for piece: " << pieceIndexForDownload << std::endl;

                //peerPieceInfo[pieceIndexForDownload] = "integrity error";

                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    torrentContent.piecesToBeDownloadedSet.insert(pieceIndexForDownload);
                    torrentContent.piecesQueue.erase(pieceIndexForDownload);
                    peer.problemDownloadingSet.insert(pieceIndexForDownload);
                    
                }
                
            
            }
            else {
                //std::cout << "piece integrity good!!" << pieceIndexForDownload << std::endl;
                torrentContent.downloadedPieces.push_back(pieceIndexForDownload);
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    torrentContent.piecesQueue.erase(pieceIndexForDownload);
                    torrentContent.fileBuiltPieces[pieceIndexForDownload] = fullPieceData;
                }
                if (sendHaveMsg(clientSocket,pieceIndexForDownload) == -1) {
                    std::cout << "sending have for piece: " << pieceIndexForDownload << "failed :(" << std::endl;
                };
                //std::cout << "successfully downloaded piece: " << i << std::endl;
                //std::cout << "pieces to be downloaded set: (";
                //for (const int &pieceIndex : torrentContent.piecesToBeDownloadedSet) {
                    //std::cout << pieceIndex << " ";
                //}
                //std::cout << ")" << std::endl;
                //std::cout << "data size: " << torrentContent.fileBuiltPieces[pieceIndexForDownload].size() << std::endl;
                
                std::cout << "Downloaded piece: " << pieceIndexForDownload << " from peer: " << peer.ip << ":" << peer.port << " |"
                << std::setprecision(4) << (static_cast<float>(torrentContent.downloadedPieces.size()) / static_cast<float>(torrentContent.numOfPieces)) * 100 
                << "%" << std::endl;

                std::cout << "(";
                for (auto const& pieceData: torrentContent.fileBuiltPieces) {
                    std::cout << pieceData.first << " ";
                }
                std::cout << ")" << std::endl;


                //std::cout << "size of array of pieces needed left: " << torrentContent.piecesToBeDownloadedSet.size() << std::endl;

            }
            std::fill(fullPieceData.begin(), fullPieceData.end(), 0);
        } else {
            std::cout << peer.ip << ":" << peer.port << " has no more pieces!" << std::endl;
            return torrentContent.downloadedPieces.size();
        }
    }
    
    return torrentContent.downloadedPieces.size();
}


void communicateWithPeers(announceProperties& announceContent, torrentProperties& torrentContents, peer& peer, std::mutex& queueMutex) {


    peerStatus peerMsg;
        int clientSocket;
            //while(torrentContents.fileBuiltPieces.size() < torrentContents.numOfPieces){
                clientSocket = connectToPeer(peer, torrentContents);
                if (clientSocket != -1) {
                    std::vector<unsigned char> handshake = createHandshake(torrentContents);

                    if (sendPeerHandshake(clientSocket, handshake, peer) != -1) {
                        peerStatus peerStatusInfo = readPeerBitfield(clientSocket, peer);
                        if (peerStatusInfo.bitFieldLength > 0) {
                            if(sendInterestedMsg(clientSocket) != -1) {
                                if(waitForUnchoke(clientSocket, peerStatusInfo) != -1) {
                                    std::cout << "not choked! yay!" << std::endl;
                                    //std::map<int, std::string> peerPieces = PeerPieceInfo(peerStatusInfo.bitfield, torrentContents.numOfPieces);
                                    while(torrentContents.fileBuiltPieces.size() < torrentContents.numOfPieces) {
                                        int downloadStatus = downloadAvailablePieces(clientSocket, torrentContents, queueMutex, peerStatusInfo, peer);
                                        if (torrentContents.fileBuiltPieces.size() == torrentContents.numOfPieces) {
                                            std::cout << "download finished!! yippe!!!" << std::endl;
                                        }
                                        if (downloadStatus == -1) {
                                            return;
                                        }
                                    }
                                
                                }
                            } 
                        } 
                    } 
                } 
            //}
        close(clientSocket);
        return;


}