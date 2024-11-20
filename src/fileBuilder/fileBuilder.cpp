#include "fileBuilder.h"
#include "utils/utils.h"
#include <fstream>
#include <filesystem>
#include <iostream>

/*
    Takes all pieces parsed, and builds the file from them
*/

bool buildFile(std::map<int, std::vector<unsigned char>>& filePiecesData, std::string& fileName) {
    int pieceAddedCounter = 0;
    std::cout << "building the file: " << fileName << std::endl;

    std::string fileDirectory = "./downloaded_files";

    std::filesystem::create_directories(fileDirectory);

    std::ofstream ofs;
    ofs.open(fileDirectory + "/" + fileName);

    if (!ofs.is_open()) {
        std::cerr << "Could not open the file to copy data to: " << strerror(errno) << std::endl;
        return false;
    }
    
    for (auto const& pieceData: filePiecesData) {
        std::cout << "adding piece: " << pieceData.first << " to file: " << fileName << std::endl;
        for (int i = 0; i < pieceData.second.size(); i++) {
            ofs << pieceData.second[i];
        }
        pieceAddedCounter++;
        displayProgressBar(pieceAddedCounter, filePiecesData.size());
    }
    ofs.close();
    return true;
}