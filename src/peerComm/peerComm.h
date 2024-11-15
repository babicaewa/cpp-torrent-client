#include <string>
#include "bencode/bdecode.h"

#ifndef PEERCOMM_H
#define PEERCOMM_H

const int peerChoked = 0;
const int peerUnchoked = 1;
const int peerInterested = 2;
const int peerNotInterested = 3;
const int peerHave = 4;
const int peerBitfield = 5;
const int peerRequest = 6;
const int peerPiece = 7;
const int peerCancel = 8;


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

int downloadAvailablePieces(std::map<int, std::string>& peerPieceInfo, int& clientSocket, torrentProperties& torrentContent, std::mutex& queueMutex, peerStatus& peerStatusInfo, peer& peer);

void communicateWithPeers(announceProperties& announceContent, torrentProperties& torrentContents, peer& peer, std::mutex& queueMutex);

#endif



