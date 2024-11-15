#include "fileBuilder.h"
#include <fstream>
#include <filesystem>
#include <iostream>


bool buildFile(std::map<int, std::vector<unsigned char>>& filePiecesData, std::string& fileName) {
    std::cout << "building the file: " << fileName << std::endl;

    std::ofstream ofs(fileName);

    if (!ofs.is_open()) {
        std::cerr << "Could not open the file to copy data to: " << strerror(errno) << std::endl;
        return false;
    }
    
    for (auto const& pieceData: filePiecesData) {
        std::cout << "adding piece: " << pieceData.first << std::endl;
        for (int i = 0; i < pieceData.second.size(); i++) {
            ofs << pieceData.second[i];
        }
    }
    ofs.close();
    return true;
}