#include <sys/socket.h>
#include "sha1/sha1.h"
#include "peerComm.h"
#include "utils/utils.h"
#include "handshake/handshake.h"
#include "messages.h"
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
const int MAX_SIMULTANEOUS_REQUESTS = 16;   //change to whatever value you want, 16 lets you request 256kb (or in some cases a whole piece) at once
const ssize_t EXPECTED_MSG_SIZE = MAX_BLOCK_SIZE + 13;
const int MAX_WAIT_TIME = 20;

/*
   Connects to a peer, timesout after 5 seconds
*/

int connectToPeer(peer& peer, torrentProperties& torrentContents) {


    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);    
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
        std::cerr << "Connection error " << peer.ip << " -> " << strerror(errno) << std::endl;
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

/*
    Sends MAX_SIMULTANEOUS_REQUESTS or how many pieces needed left (minimum of the two) to a peer, if the last piece is a partial piece, then calculates the size needed to ask for
*/

int sendRequestMsgs(int& clientSocket, int& pieceIndexForDownload, int& startIndex, int endIndex, torrentProperties& torrentContent) {
    std::vector<unsigned char> requestMsg;
    for (int i = startIndex; i < endIndex; i++) {
        if (torrentContent.partialPieceSize != 0 && i == endIndex - 1 && pieceIndexForDownload == torrentContent.numOfPieces - 1) {
            requestMsg = createRequestMsg(pieceIndexForDownload, i*MAX_BLOCK_SIZE, (torrentContent.partialPieceSize % MAX_BLOCK_SIZE));
        } else {
            requestMsg = createRequestMsg(pieceIndexForDownload, i*MAX_BLOCK_SIZE, MAX_BLOCK_SIZE);
        } 
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

/*
   Downloads all of the requested pieces sent by the peer
*/

int downloadRequestedPieceBlocks(int& clientSocket, int& pieceIndexForDownload, int& startIndex, int endIndex, std::vector<unsigned char>& fullPieceData, torrentProperties& torrentContent, std::mutex& queueMutex, peerStatus& peerStatusInfo, peer& peer) {
    std::vector<unsigned char> pieceBlockBuffer;  //does not account for partial piece!!!
    for (int i = startIndex; i < endIndex; i++) {
        if (torrentContent.partialPieceSize != 0 && i == endIndex - 1 && pieceIndexForDownload == torrentContent.numOfPieces - 1) {
            pieceBlockBuffer.resize((torrentContent.partialPieceSize % MAX_BLOCK_SIZE) + 13);
        }
        else {
            pieceBlockBuffer.resize(EXPECTED_MSG_SIZE,0);
        }
       auto startTime = std::chrono::steady_clock::now();

        ssize_t bytesRead = 0;
        while (bytesRead < pieceBlockBuffer.size()) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
            if (elapsedSeconds > MAX_WAIT_TIME) {
                break;
            }
            ssize_t peerBlockRepsonse = read(clientSocket, pieceBlockBuffer.data()+bytesRead, EXPECTED_MSG_SIZE-bytesRead);
            if (peerBlockRepsonse < 0) {
                return -1;
            }
            bytesRead += peerBlockRepsonse;
        }

        if (static_cast<int>(pieceBlockBuffer[4]) == 7) {
            int pieceOffset = static_cast<uint32_t>(pieceBlockBuffer[9]) << 24 |
                            static_cast<uint32_t>(pieceBlockBuffer[10]) << 16 |
                            static_cast<uint32_t>(pieceBlockBuffer[11]) << 8 |
                            static_cast<uint32_t>(pieceBlockBuffer[12]);   
            
            if ((pieceOffset + pieceBlockBuffer.size()-14) >= fullPieceData.size()) {
                break;
            }
            std::copy(pieceBlockBuffer.begin()+13, pieceBlockBuffer.end(),fullPieceData.begin()+pieceOffset);
            
            std::fill(pieceBlockBuffer.begin(), pieceBlockBuffer.end(), 0);
            }
            else if (static_cast<int>(pieceBlockBuffer[4]) == 0)  {
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    torrentContent.piecesToBeDownloadedSet.insert(pieceIndexForDownload);
                    torrentContent.piecesQueue.erase(pieceIndexForDownload);
                }
                peerStatusInfo.choked = true;
                if (waitForUnchoke(clientSocket, peerStatusInfo, peer) == -1) {
                    return -1;
                };

            }

    }
    return 0;
}

/*
   Downloads all pieces that a peer has
*/

int downloadAvailablePieces(int& clientSocket, torrentProperties& torrentContent, std::mutex& queueMutex, peerStatus& peerStatusInfo, peer& peer) {
    ssize_t bytesRead;
    std::vector<unsigned char> fullPieceData(torrentContent.pieceLength, 0);
    std::vector<unsigned char> requestMsg;
    int pieceIndexForDownload = 0;
    int blockRequestIndex;
    int totalBlocksNeeded;

    while(torrentContent.fileBuiltPieces.size() < torrentContent.numOfPieces) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            pieceIndexForDownload = -1;
            auto queueIterator = torrentContent.piecesToBeDownloadedSet.begin();
            while (queueIterator != torrentContent.piecesToBeDownloadedSet.end()) {
                int piece = *queueIterator;
                if (peerHasPiece(peerStatusInfo.bitfield, piece) && torrentContent.fileBuiltPieces.find(piece) == torrentContent.fileBuiltPieces.end() 
                && torrentContent.piecesQueue.find(piece) == torrentContent.piecesQueue.end()
                && peer.problemDownloadingSet.find(piece) == peer.problemDownloadingSet.end()) {
                    pieceIndexForDownload = piece;
                    torrentContent.piecesToBeDownloadedSet.erase(queueIterator);
                    torrentContent.piecesQueue.insert(piece);
                    break;
                }
                ++queueIterator;
            }
        }
        if(pieceIndexForDownload != -1) {
            if (torrentContent.partialPieceSize != 0 && pieceIndexForDownload == torrentContent.numOfPieces -1) {
                fullPieceData.resize(torrentContent.partialPieceSize);
            } else {
                fullPieceData.resize(torrentContent.pieceLength);
            }
            blockRequestIndex = 0;
            if(pieceIndexForDownload == torrentContent.numOfPieces-1 && torrentContent.partialPieceSize != 0) {
                totalBlocksNeeded = std::ceil(static_cast<float>(torrentContent.partialPieceSize)/static_cast<float>(MAX_BLOCK_SIZE));  //if last piece is a partial piece, then calculates how many blocks to request for last block
            } else {
                totalBlocksNeeded = torrentContent.pieceLength/MAX_BLOCK_SIZE;
            }
            while(blockRequestIndex < totalBlocksNeeded) {
                int endBlockIndex = std::min(blockRequestIndex+MAX_SIMULTANEOUS_REQUESTS, totalBlocksNeeded);
                if(sendRequestMsgs(clientSocket,pieceIndexForDownload,blockRequestIndex,endBlockIndex, torrentContent) == 0) {
                    if (downloadRequestedPieceBlocks(clientSocket, pieceIndexForDownload, blockRequestIndex, endBlockIndex, fullPieceData, torrentContent, queueMutex, peerStatusInfo, peer) == -1) {
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
            std::string downloadedPieceHashBinary = decodeHashStringToBinary(downloadedPieceHash);
            if (downloadedPieceHashBinary != torrentContent.pieceHashes[pieceIndexForDownload]) {
                std::cout << "piece integrity bad!! for piece: " << pieceIndexForDownload << std::endl;

                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    torrentContent.piecesToBeDownloadedSet.insert(pieceIndexForDownload);
                    torrentContent.piecesQueue.erase(pieceIndexForDownload);
                    peer.problemDownloadingSet.insert(pieceIndexForDownload);
                    
                }
                
            
            }
            else {
                torrentContent.downloadedPieces.push_back(pieceIndexForDownload);
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    torrentContent.piecesQueue.erase(pieceIndexForDownload);
                    torrentContent.fileBuiltPieces[pieceIndexForDownload] = fullPieceData;
                }
                if (sendHaveMsg(clientSocket,pieceIndexForDownload) == -1) {
                    std::cout << "sending have for piece: " << pieceIndexForDownload << "failed :(" << std::endl;
                };
                std::cout << "Downloaded piece: " << pieceIndexForDownload << " from peer: " << peer.ip << ":" << peer.port << std::endl;
                displayProgressBar(static_cast<float>(torrentContent.downloadedPieces.size()), static_cast<float>(torrentContent.numOfPieces));
            }
            std::fill(fullPieceData.begin(), fullPieceData.end(), 0);
        } else {
            return torrentContent.downloadedPieces.size();
        }
    }
    
    return torrentContent.downloadedPieces.size();
}

/*
   Main logic for connecting to peers, handshaking, requesting pieces, and downloading pieces
*/

void communicateWithPeers(announceProperties& announceContent, torrentProperties& torrentContents, peer& peer, std::mutex& queueMutex) {

    peerStatus peerMsg;
        int clientSocket;
                std::cout << "establishing connection with: " << peer.ip << ":" << peer.port << std::endl;
                clientSocket = connectToPeer(peer, torrentContents);
                if (clientSocket != -1) {
                    std::vector<unsigned char> handshake = createHandshake(torrentContents);
                    if (sendPeerHandshake(clientSocket, handshake, peer) != -1) {
                        peerStatus peerStatusInfo = readPeerBitfield(clientSocket, peer);
                        if (peerStatusInfo.bitFieldLength > 0) {
                            if(sendInterestedMsg(clientSocket) != -1) {
                                if(waitForUnchoke(clientSocket, peerStatusInfo, peer) != -1) {
                                    while(torrentContents.fileBuiltPieces.size() < torrentContents.numOfPieces) {
                                        peer.problemDownloadingSet.clear();
                                        sleep(1); //to let other peers download stick pieces
                                        int downloadStatus = downloadAvailablePieces(clientSocket, torrentContents, queueMutex, peerStatusInfo, peer);
                                        if (torrentContents.fileBuiltPieces.size() == torrentContents.numOfPieces) {
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
        close(clientSocket);
        return;


}