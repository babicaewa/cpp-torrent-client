#include <string>
#include <map>
#include <vector>
#include <set>
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>

#ifndef BDECODE_H
#define BDECODE_H


struct torrentProperties {
    std::string announce;
    std::vector<std::string> announceList;
    std::string comment;
    std::string createdBy;
    long creationDate;
    long length;
    std::string name;
    long pieceLength;
    std::string pieces;
    int numOfPieces;
    int partialPieceSize;
    std::string infoHash;
    std::string peerID;
    std::vector<std::string> pieceHashes;
    std::map <int, std::vector<unsigned char>> fileBuiltPieces; //i = piece number, j = piece bytes
    std::set<int> piecesQueue;
    std::set<int> piecesToBeDownloadedSet;
    std::vector<int> downloadedPieces;
};


struct peer {
    std::string ip;
    int port;
    std::set<int> problemDownloadingSet;
};

struct announceProperties {
    int interval;
    std::vector<peer> peers;
};

std::string torrentToString(std::string filePath);

std::string decodeInt(std::string& torrentFileString, int& i);

std::string decodeString(std::string& torrentFileString, int& i);

std::vector<std::string> createPieceHashArray(int& numOfPieces, std::string piecesString);

torrentProperties decodeTorrent(std::string filePath);

announceProperties decodeAnnounceResponse(std::string& trackerResponse);

std::vector<peer> decodePeers(std::string& announceContentString);


#endif