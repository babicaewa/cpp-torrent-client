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
    std::map<std::string, std::string> contentMap;
    std::string infoHash;
};

std::string torrentToString(std::ifstream torrentFile);

std::string decodeInt(std::string& torrentFileString);

std::string decodeString(std::string& torrentFileString);

torrentProperties decodeTorrent();


#endif