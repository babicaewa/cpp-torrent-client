#include <string>
#include "bencode/bdecode.h"

#ifndef PEERCOMM_H
#define PEERCOMM_H

struct peerStatus {
    peer peer;
    uint32_t bitFieldLength;
    std::vector<unsigned char> bitfield;
    bool choked;
};

std::vector<unsigned char> decodeHashtoPureHex(std::string hashedString);

std::string decodeHashToBinary(std::string& hashString);

std::vector<unsigned char> createHandshake(torrentProperties& torrentContents);

peerStatus readPeerBitfield(int& clientSocket, peer& peer);

int waitForUnchoke(int& cilentSocket, peerStatus& peerStatusInfo);

peerStatus parsePeerMessage(std::vector<unsigned char>& response_buffer, peer& peer);

bool peerHasPiece(std::vector<unsigned char>& bitfield, int& index);

std::map<int, std::string>PeerPieceInfo(std::vector<unsigned char>& bitfield, int& totalPieces);

int connectToPeer(peer& peer, torrentProperties& torrentContents);

int sendPeerHandshake(int& clientSocket, std::vector<unsigned char> handshake, peer& peer);

int sendInterestedMsg(int& clientSocket);

std::vector<unsigned char> createRequestMsg(int piece, int beginIndex, int blockLength);

int downloadRequestedPieceBlocks(int& clientSocket, int& pieceIndexForDownload, int& startIndex, int endIndex, std::vector<unsigned char>& fullPieceData, torrentProperties& torrentContent, std::mutex& queueMutex, peerStatus& peerStatusInfo, peer& peer);

int downloadAvailablePieceBlocks(std::map<int, std::string>& peerPieceInfo, int& clientSocket, torrentProperties& torrentContent, std::mutex& queueMutex, peerStatus& peerStatusInfo, peer& peer);

void communicateWithPeers(announceProperties& announceContent, torrentProperties& torrentContents, peer& peer, std::mutex& queueMutex);

#endif



