#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>
#ifndef BENCODER_H
#define BENCODER_H

struct torrentInfo {
    std::string announce;
    std::string comment;
    std::string createdBy;
    int creationDate;
    int length;
    std::string fileName;
    int pieceLength;
    int numOfPieces;
    std::vector<std::vector<unsigned char> > sha1Pieces; //resize to numOfPieces .resize
};


std::string convertFileToString() {

    std::ifstream torrentFile("../res/debian-12.7.0-s390x-netinst.iso.torrent"); //need to change evenutally

    std::ostringstream oss;
    oss << torrentFile.rdbuf(); 

    std::string fileString = oss.str(); //puts the whole file into a string

    torrentFile.close();
    
    std::cout << "file str length: " << fileString.length() << std::endl;

    return fileString;
}

int grabStrLengthEndIndex(std::string torrentString, int j) {

    while (torrentString[j] != ':') j++;            
    return j;

}

int grabIntValEndIndex(std::string torrentString, int j) {
    while (torrentString[j] != 'e') { 
        std::cout << "char: " << torrentString[j] << std::endl;
        j++;
    }
    std::cout << "endindex: " << j << std::endl;

    return j;
}



void decodeTorrent() {
    std::ifstream torrentFile;
    std::string fileLine;

    std::string torrentFileString = convertFileToString();

    torrentInfo torrent;

    std::map<std::string, std::string> infoMap;
    int i = 0;
    int strLenEndIndex;
    int strLength;
    int intValEndIndex;
    std::string infoSubStr;

    std::string torrentKey = "";
    std::string torrentVal = "";

    std::cout << torrentFileString << std::endl;
        while (i < torrentFileString.length()) {
            if (torrentFileString[i] == 'd' || torrentFileString[i] == 'l' || torrentFileString[i] == 'e') {
                i++;
            } else {
                if (torrentFileString[i] >= '0' && torrentFileString[i] <= '9') {
                    strLenEndIndex = grabStrLengthEndIndex(torrentFileString, i);
                    strLength = stoi(torrentFileString.substr(i, strLenEndIndex-1));        //if the value is a string
                    i = strLenEndIndex + 1;

                    infoSubStr = torrentFileString.substr(i, strLength);;
                    if (infoSubStr != "info") {
                        if (torrentKey == "") torrentKey = infoSubStr;
                        else torrentVal = infoSubStr; 
                    }                 

                    i += strLength;

                } else {        //if value is an integer
                    intValEndIndex = grabIntValEndIndex(torrentFileString, i);
                    torrentVal = torrentFileString.substr(i+1,intValEndIndex-i-1);
                    i = intValEndIndex + 1;
                }

            }
            std::cout <<"torrentKey: "<< torrentKey << std::endl;
            std::cout <<"torrentVal: "<< torrentVal << std::endl;
            if (torrentVal != "") {
                infoMap[torrentKey] = torrentVal;
                torrentKey = "";
                torrentVal = "";
            }
        }

            // Iterate and print the map
            std::cout << std::endl;
            std::cout << "map {" << std::endl;
    for (const auto &pair : infoMap) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    std::cout << "}" << std::endl;

    return;
};

#endif