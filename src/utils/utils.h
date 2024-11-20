#include <vector>
#include <string>

#ifndef UTILS_H
#define UTILS_H

void displayProgressBar(float currVal, float totalVal);

std::string generatePeerID();

std::string hashToURLEncoding(std::string& hashedString);

std::vector<unsigned char> decodeHashStringtoPureHex(std::string hashedString);

std::string decodeHashStringToBinary(std::string& hashString);

#endif