#include <iostream>
#include "bencode/bdecode.h"

int main() {
  torrentProperties torrentContent = decodeTorrent();
  

  std::cout << std::endl;
  std::cout << "map {" << std::endl;
    for (const auto &pair : torrentContent.contentMap) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    std::cout << "}" << std::endl;
  std::cout << "infohash: " << std::endl;
  std::cout << torrentContent.infoHash << std::endl;
} 