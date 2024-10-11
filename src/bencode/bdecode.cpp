#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>
#include "bdecode.h"

int i = 0;

std::string torrentToString() {

    std::ifstream torrentFile("../res/debian-12.7.0-s390x-netinst.iso.torrent"); //need to change evenutally

    std::ostringstream oss;
    oss << torrentFile.rdbuf(); 

    std::string fileString = oss.str(); //puts the whole file into a string

    torrentFile.close();
    
    std::cout << "file str length: " << fileString.length() << std::endl;

    return fileString;

};

std::string decodeInt(std::string& torrentFileString) {
    int j = i;
    while (torrentFileString[j] != 'e') {
        j++;
    }
    std::string decodedInt = torrentFileString.substr(i + 1, j-i-1);
    i = j + 1;
    return decodedInt;
};

std::string decodeString(std::string& torrentFileString) {
    int j = i;
    while (torrentFileString[j] != ':') {
        j++;
    }
    int stringLength = stoi(torrentFileString.substr(i,j));
    i = j + stringLength + 1;
    return torrentFileString.substr(j+1, stringLength);
}

torrentProperties decodeTorrent() {
    std::string torrentFileString = torrentToString();

    torrentProperties torrentContents;

    std::string torrentContentString;

    std::string infoHash;
    int infoHashStartIndex;
    int infoHashEndIndex;

    std::string mapKey = "";
    std::string mapVal = "";
    
    while (i < torrentFileString.length()-1) {
        if (torrentFileString[i] == 'd' || torrentFileString[i] == 'e' || torrentFileString[i] == 'l') {
            i++;
        }
        else {
            if (torrentFileString[i] >= '0' && torrentFileString[i] <= '9') {
                torrentContentString = decodeString(torrentFileString);         //if a bencoded string is encountered
            }
            else if (torrentFileString[i] == 'i') {
                torrentContentString = decodeInt(torrentFileString);             //if a bencoded int is encountered
            }

            if (torrentContentString == "info") {
                infoHashStartIndex = i - 6;
            }
            else if (mapKey == "") {
                mapKey = torrentContentString;
            } 
            else if (mapVal == "") {
                if (mapKey == "pieces") {
                    infoHashEndIndex = i;
                }
                mapVal = torrentContentString;
                torrentContents.contentMap[mapKey] = mapVal;
                mapKey = "";
                mapVal = "";
            } 
        }

    torrentContents.infoHash = torrentFileString.substr(infoHashStartIndex, infoHashEndIndex-infoHashStartIndex+1);
    }
    //torrentContents.infoHash = torrentFileString[infoHashEndIndex];
    //std::cout << torrentFileString[infoHashStartIndex] << std::endl;
    return torrentContents;
};