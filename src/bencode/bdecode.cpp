#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>
#include <stdlib.h>
#include <cmath>
#include "bdecode.h"
#include "sha1/sha1.h"
#include "trackerInfo/trackerComm.h"

const int SHA1_HASH_LENGTH = 20;

/*
    Reads the torrent File, and converts the file into a string
*/

std::string torrentToString(std::string filePath) {

    std::ifstream torrentFile(filePath);
    std::ostringstream oss;
    oss << torrentFile.rdbuf(); 

    std::string fileString = oss.str(); 

    torrentFile.close();

    if (fileString.length() <= 0) {
        std::cout << "error reading the torrent file or file doesn't exist." << std::endl;
        exit(-1);
    }

    return fileString;

};

/*
    Decodes bencode int to a c++ string
*/

std::string decodeInt(std::string& torrentFileString, int& i) {
    int j = i;
    while (torrentFileString[j] != 'e') {
        j++;                               
    }
    std::string decodedInt = torrentFileString.substr(i + 1, j-i-1);
    i = j + 1;
    return decodedInt;
};

/*
    Decodes bencode string to a c++ string
*/

std::string decodeString(std::string& torrentFileString, int& i) {
    int j = i;
    while (torrentFileString[j] != ':') {              
        j++;
    }
    int stringLength = stoi(torrentFileString.substr(i,j));
    i = j + stringLength + 1;
    return torrentFileString.substr(j+1, stringLength);
}

/*
    Decodes bencode list to a vector
*/

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

/*
    Takes the .torrent file's piece hashes, and put them into a vector
*/

std::vector<std::string> createPieceHashArray(int& numOfPieces, std::string piecesString) {
    std::vector<std::string> pieceHashArray(numOfPieces);

    for (int i = 0; i < numOfPieces; i++) {
        pieceHashArray[i] = piecesString.substr(i*20, 20);
    }
    return pieceHashArray;
}

/*
    grabs ip and ports of each peer in the announce response
*/

std::vector<peer> decodePeers(std::string& announceContentString) { 
    int ipIndex = 0;                        
    std::vector<peer> peers;
    std::string ipString;
    int ipPort;
    peer peer;


    while (ipIndex < announceContentString.length()) {
        ipString += std::to_string(int(static_cast<unsigned char>(announceContentString[ipIndex]))) + '.'
                + std::to_string(int(static_cast<unsigned char>(announceContentString[ipIndex + 1]))) + '.'
                + std::to_string(int(static_cast<unsigned char>(announceContentString[ipIndex + 2]))) + '.'
                + std::to_string(int(static_cast<unsigned char>(announceContentString[ipIndex + 3])));
        ipPort = (int(static_cast<unsigned char>(announceContentString[ipIndex + 4])) << 8)
                             | int(static_cast<unsigned char>(announceContentString[ipIndex + 5]));
        peer.ip = ipString;
        peer.port = ipPort;
        std::set<int> set;
        peer.problemDownloadingSet = set;

        peers.push_back(peer);
        ipString = "";
        ipIndex += 6;
    }
    return peers;
}

/*
    Decodes whole torrent file, and puts it into a "torrentProperties" typedef
*/


torrentProperties decodeTorrent(std::string filePath) {
    int i = 0;

    std::string torrentFileString = torrentToString(filePath);

    torrentProperties torrentContents;

    std::string torrentContentString;      
    std::vector<std::string> torrentContentList;

    std::string infoHash;
    int infoHashStartIndex = -1;
    int infoHashEndIndex;

    std::string torrentDataSection = "";    
    
    while (i < torrentFileString.length()-1) {
        if (torrentFileString[i] == 'd') {
            i++;
        }
        else if (torrentFileString[i] == 'e') {
            if (infoHashStartIndex != -1) {
                infoHashEndIndex = i;
            }
            i++;
        }
        else {
            if (torrentFileString[i] >= '0' && torrentFileString[i] <= '9') {
                torrentContentString = decodeString(torrentFileString, i);       
            }
            else if (torrentFileString[i] == 'i') {
                torrentContentString = decodeInt(torrentFileString, i);        
            }
            else if (torrentFileString[i] == 'l') {
                //std::cout << "list dtart!!!!: " << i << std::endl;
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
                else if (torrentDataSection == "announce-list" || torrentDataSection == "url-list") {
                    torrentContents.announceList = torrentContentList;
                }
                else if (torrentDataSection == "comment") {
                    torrentContents.comment = torrentContentString;
                }
                else if (torrentDataSection == "created by") {
                    torrentContents.createdBy = torrentContentString;
                }
                else if (torrentDataSection == "creation date") {
                    torrentContents.creationDate = std::stol(torrentContentString);
                }
                else if (torrentDataSection == "length") {
                    torrentContents.length = std::stol(torrentContentString);
                }
                else if (torrentDataSection == "name") {
                    torrentContents.name = torrentContentString;
                }
                else if (torrentDataSection == "piece length") {
                    torrentContents.pieceLength = std::stol(torrentContentString);
                }
                else if (torrentDataSection == "pieces") {
                    torrentContents.pieces = torrentContentString;
                
                } 
                torrentDataSection = "";
            } 
        }


    }
    torrentContents.infoHash = sha1Hash(torrentFileString.substr(infoHashStartIndex, infoHashEndIndex-infoHashStartIndex+1));

    torrentContents.numOfPieces = std::ceil(static_cast<float>(torrentContents.length)/static_cast<float>(torrentContents.pieceLength));
    
    torrentContents.partialPieceSize = torrentContents.length % torrentContents.pieceLength;

    torrentContents.pieceHashes = createPieceHashArray(torrentContents.numOfPieces, torrentContents.pieces);

    std::map <int, std::vector<unsigned char>> fileBuiltPieces;
    torrentContents.fileBuiltPieces = fileBuiltPieces;
    std::set<int> pq;
    torrentContents.piecesQueue = pq;


    std::set<int> piecesToBeDownloadedSet;
    for (int j = 0; j < torrentContents.numOfPieces; j++) {
        piecesToBeDownloadedSet.insert(j);
    }

    torrentContents.piecesToBeDownloadedSet = piecesToBeDownloadedSet;
    //torrentContents.infoHash = torrentFileString[infoHashEndIndex];
    //std::cout << "stsart index: " << torrentFileString[infoHashStartIndex] << std::endl;
    return torrentContents;
};

/*
    Decodes bencode announce response
*/

announceProperties decodeAnnounceResponse(std::string& announceString) {
    int j = 0;
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
                announceContentString = decodeInt(announceString, j);
            }
            
            if (announceDataSection == "") {
                announceDataSection = announceContentString;
            }
            else {
                if (announceDataSection == "interval") {
                    announceContents.interval = std::stoi(announceContentString);
                }
                else if (announceDataSection == "peers") {
                    announceContents.peers = decodePeers(announceContentString);
                }
                announceDataSection = "";
            }
        }
    }
    return announceContents;
};