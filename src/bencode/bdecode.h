#include <string>
#include <map>
#include <vector>
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
    int creationDate;
    int length;
    std::string name;
    int pieceLength;
    std::string pieces;
    std::string infoHash;
};

struct peerData {
    std::string ip;
    int port;
};

struct announceProperties {
    int interval;
    std::vector<peerData> peers;
};

std::string torrentToString(std::ifstream torrentFile, int& i);

std::string decodeInt(std::string& torrentFileString, int& i);

std::string decodeString(std::string& torrentFileString, int& i);

torrentProperties decodeTorrent();

announceProperties decodeAnnounceResponse(std::string& trackerResponse);

std::vector<peerData> decodePeers(std::string& announceContentString);


#endif