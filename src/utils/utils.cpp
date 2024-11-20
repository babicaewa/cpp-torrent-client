#include "utils/utils.h"
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

/*
    Displays a nicely formatted progress bar
*/

void displayProgressBar(float currVal, float totalVal) {
    float progressPercentage = std::min((currVal / totalVal) * 100,100.0f);
    int progressBarLength = 100;

    std::cout << "[";
    for (int i=0; i < progressPercentage; i++) {
        std::cout << "=";
    }
    std::cout << ">";
    for (int i=progressPercentage+1; i < progressBarLength; i++) {
        std::cout << ".";
    }
    std::cout << "] " << std::setprecision(4) << progressPercentage << "%" <<std::endl;

    return;
}     

/*
    Creates a random PeerID for the client
*/

std::string generatePeerID() {
    std::stringstream peerID;
    unsigned char hexVal;
    for (int j = 0; j < 20; ++j) {
        hexVal = 97 + rand() % 25;
        peerID << std::hex << std::setw(2) << std::setfill('0') << (int)hexVal;
    }
    return peerID.str();
}

/*
    Takes a SHA-1 hash and converts it to a url readable format for the tracker
*/

std::string hashToURLEncoding(std::string& hashedString) {
    std::string urlHash;

    for (int i = 0; i < hashedString.length(); i += 2) {
        urlHash += '%';
        urlHash += hashedString.substr(i, 2);
    }
    return urlHash;
}

/*
    Converts a SHA-1 hash to pure hex (since SHA-1 function returns a string)
*/

std::vector<unsigned char> decodeHashStringtoPureHex(std::string hashedString) {
    std::vector<unsigned char> decodedHash;
    std::string subStringByte;
    for (int i = 0; i < hashedString.length(); i += 2) {
        unsigned char byte = static_cast<unsigned char>(std::stoi(hashedString.substr(i, 2), nullptr, 16));
        decodedHash.push_back(byte);
    };

    return decodedHash;
}

/*
    Converts a SHA-1 hash to binary (since SHA-1 function returns a string)
*/

std::string decodeHashStringToBinary(std::string& hashString) {
    std::string hexStr;
    std::string binaryStr;
    for (int i = 0; i < hashString.length(); i+= 2) {
        hexStr = hashString.substr(i, 2);
        binaryStr += (static_cast<unsigned char>(std::strtol(hexStr.c_str(), nullptr,16)));
    }
    return binaryStr;
}