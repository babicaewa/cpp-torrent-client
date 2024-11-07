#include <iostream>
#include "bencode/bdecode.h"
#include "trackerInfo/trackerComm.h"
#include "peerComm/peerComm.h"
#include <thread>

  // Function to print torrentProperties struct
  void printTorrentProperties(const torrentProperties &tp) {
      // Print single properties
      std::cout << "Announce: " << tp.announce << std::endl;
      std::cout << "Comment: " << tp.comment << std::endl;
      std::cout << "Created By: " << tp.createdBy << std::endl;
      std::cout << "Creation Date: " << tp.creationDate << std::endl;
      std::cout << "Length: " << tp.length << std::endl;
      std::cout << "Name: " << tp.name << std::endl;
      std::cout << "Piece Length: " << tp.pieceLength << std::endl;
      std::cout << "Number of pieces: " << tp.numOfPieces << std::endl;
      //std::cout << "Pieces: " << tp.pieces << std::endl;
      std::cout << "Info Hash: " << tp.infoHash << std::endl;

      // Print announce list (vector)
      std::cout << "Announce List: " << tp.announceList.size() << std::endl;
      for (const auto &listItem : tp.announceList) {
          std::cout << "  - " << listItem << std::endl;
      }
  };

void printAnnounceProperties(const announceProperties &ap) {
  std::cout << "Interval: " << ap.interval << std::endl;
  std::cout << "Peers: " << ap.peers.size() << std::endl;
  for (const auto &listItem : ap.peers) {
          std::cout << "IP: " << listItem.ip << " Port: " << listItem.port << std::endl;
      }
}

int main() {
  std::ifstream testFile("../res/debian-mac-12.7.0-amd64-netinst.iso.torrent");

  torrentProperties torrentContent = decodeTorrent();
  std::string response = communicateToTracker(torrentContent);
  announceProperties announceContent = decodeAnnounceResponse(response);

  printTorrentProperties(torrentContent);
  printAnnounceProperties(announceContent);

  std::vector<std::thread> peerThreads(announceContent.peers.size());
  for (int i= 0; i < announceContent.peers.size(); i++) {
    peerThreads[i] = std::thread(communicateWithPeers, std::ref(announceContent), std::ref(torrentContent), std::ref(announceContent.peers[i]));
    //peerThreads[i].join();
  }
  for (int j=0; j < announceContent.peers.size(); j++) {
    peerThreads[j].join();
  }

  //communicateWithPeers(announceContent, torrentContent);
  
  
};