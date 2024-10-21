#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>
#include "bdecode.h"
#include "sha1/sha1.h"
#include "trackerInfo/trackerComm.h"

std::string torrentToString() {

    std::ifstream torrentFile("/Users/Alex/Downloads/debian-mac-12.7.0-amd64-netinst.iso.torrent"); //need to change evenutally

    std::ostringstream oss;
    oss << torrentFile.rdbuf(); 

    std::string fileString = oss.str(); //puts the whole file into a string

    torrentFile.close();
    
    std::cout << "file str length: " << fileString.length() << std::endl;

    return fileString;

};

std::string decodeInt(std::string& torrentFileString, int& i) {
    int j = i;
    while (torrentFileString[j] != 'e') {
        j++;
    }
    std::string decodedInt = torrentFileString.substr(i + 1, j-i-1);
    i = j + 1;
    return decodedInt;
};

std::string decodeString(std::string& torrentFileString, int& i) {
    int j = i;
    while (torrentFileString[j] != ':') {
        j++;
    }
    int stringLength = stoi(torrentFileString.substr(i,j));
    i = j + stringLength + 1;
    return torrentFileString.substr(j+1, stringLength);
}

std::vector<std::string> decodeList(std::string& torrentFileString, int& i) {
    std::string torrentListItem;
    std::vector<std::string> torrentContentList;
    std::vector<std::string> listStack;
    listStack.push_back("l");
    i++;

    while (listStack.size() != 0) {
        if (torrentFileString[i] == 'l') {
            listStack.push_back("l");
            i++;
        }
        else if (torrentFileString[i] == 'e') {
            listStack.pop_back();
            i++;
        }
        else if (torrentFileString[i] == 'i') {
            torrentListItem = decodeInt(torrentFileString, i);
            torrentContentList.push_back(torrentListItem);
        }
        else if (torrentFileString[i] >= '0' && torrentFileString[i] <= '9') {
            torrentListItem = decodeString(torrentFileString, i);
            torrentContentList.push_back(torrentListItem);
        }
    }
    return torrentContentList;
};

std::vector<peerData> decodePeers(std::string& announceContentString) {
    int index;

    while (index < announceContentString.length()) {

    }
}



torrentProperties decodeTorrent() {
    int i = 0;

    std::string torrentFileString = torrentToString();

    torrentProperties torrentContents;

    std::string torrentContentString;      
    std::vector<std::string> torrentContentList;

    std::string infoHash;
    int infoHashStartIndex;
    int infoHashEndIndex;

    std::string torrentDataSection = "";    
    
    while (i < torrentFileString.length()-1) {
        if (torrentFileString[i] == 'd' || torrentFileString[i] == 'e') {
            i++;
        }
        else {
            if (torrentFileString[i] >= '0' && torrentFileString[i] <= '9') {
                torrentContentString = decodeString(torrentFileString, i);         //if a bencoded string is encountered
            }
            else if (torrentFileString[i] == 'i') {
                torrentContentString = decodeInt(torrentFileString, i);             //if a bencoded int is encountered
            }
            else if (torrentFileString[i] == 'l') {
                std::cout << "list dtart!!!!: " << i << std::endl;
                torrentContentList = decodeList(torrentFileString, i);
            }

            if (torrentContentString == "info") {
                infoHashStartIndex = i;
            }
            else if (torrentDataSection == "") {
                torrentDataSection = torrentContentString;
            } 
            else {
                if (torrentDataSection == "announce") {
                    torrentContents.announce = torrentContentString;
                }
                else if (torrentDataSection == "announce-list") {
                    torrentContents.announceList = torrentContentList;
                }
                else if (torrentDataSection == "comment") {
                    torrentContents.comment = torrentContentString;
                }
                else if (torrentDataSection == "created by") {
                    torrentContents.createdBy = torrentContentString;
                }
                else if (torrentDataSection == "creation date") {
                    torrentContents.creationDate = std::stoi(torrentContentString);
                }
                else if (torrentDataSection == "length") {
                    torrentContents.length = std::stoi(torrentContentString);
                }
                else if (torrentDataSection == "name") {
                    torrentContents.name = torrentContentString;
                }
                else if (torrentDataSection == "piece length") {
                    torrentContents.pieceLength = std::stoi(torrentContentString);
                }
                else if (torrentDataSection == "pieces") {
                    infoHashEndIndex = i;
                    torrentContents.pieces = torrentContentString;
                    torrentContents.infoHash = sha1Hash(torrentFileString.substr(infoHashStartIndex, infoHashEndIndex-infoHashStartIndex+1));
                } 
                torrentDataSection = "";
            } 
        }


    }
    //torrentContents.infoHash = torrentFileString[infoHashEndIndex];
    std::cout << "stsart index: " << torrentFileString[infoHashStartIndex] << std::endl;
    return torrentContents;
};

announceProperties decodeAnnounceResponse(std::string& announceString) {
    int j = 0;
    std::cout << "does the function run????" << std::endl;
     std::cout << "announceString: " << announceString << std::endl;
    std::string announceContentString;  
    std::string announceDataSection = "";   
    announceProperties announceContents;

    while (j < announceString.length() ) {
        if (announceString[j] == 'd' || announceString[j] == 'e') {
            j++;
        }
        else {
            if (announceString[j] >= '0' && announceString[j] <= '9') {
                announceContentString = decodeString(announceString, j);
            }
            else if (announceString[j] == 'i') {
                std::cout << "number" << announceString[j] << std::endl;
                announceContentString = decodeInt(announceString, j);
            }
            
            if (announceDataSection == "") {
                announceDataSection = announceContentString;
            }
            else {
                if (announceDataSection == "interval") {
                     //std::cout << "Interval waa: " << announceContentString << std::endl;
                    std::cout << "Interval waa: " << announceContentString << std::endl;
                    announceContents.interval = stoi(announceContentString);
                }
                else if (announceDataSection == "peers") {
                    std::cout << "peers: " << announceContentString << std::endl;
                    //announceContents.peers = decodePeers(announceContentString);
                }
                announceDataSection = "";
            }
        }
    }
    return announceContents;
};