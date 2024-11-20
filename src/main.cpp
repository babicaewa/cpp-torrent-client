#include <iostream>
#include "bencode/bdecode.h"
#include "trackerInfo/trackerComm.h"
#include "peerComm/peerComm.h"
#include "fileBuilder/fileBuilder.h"
#include <thread>
#include <mutex>
#include <string>


/*
    Prints torrent properties to the console
*/
  void printTorrentProperties(const torrentProperties &tp) {
      std::cout << "Announce: " << tp.announce << std::endl;
      std::cout << "Comment: " << tp.comment << std::endl;
      std::cout << "Created By: " << tp.createdBy << std::endl;
      std::cout << "Creation Date: " << tp.creationDate << std::endl;
      std::cout << "Length: " << tp.length << std::endl;
      std::cout << "Name: " << tp.name << std::endl;
      std::cout << "Piece Length: " << tp.pieceLength << std::endl;
      std::cout << "Number of pieces: " << tp.numOfPieces << std::endl;
      std::cout << "Partial piece size: " << tp.partialPieceSize << std::endl;
      std::cout << "Info Hash: " << tp.infoHash << std::endl;

      std::cout << "Announce List: " << tp.announceList.size() << std::endl;
      for (const auto &listItem : tp.announceList) {
          std::cout << "  - " << listItem << std::endl;
      }
  };

/*
    Prints tracker announce properties to the console
*/

void printAnnounceProperties(const announceProperties &ap) {
  std::cout << "Interval: " << ap.interval << std::endl;
  std::cout << "Peers: " << ap.peers.size() << std::endl;
  for (const auto &listItem : ap.peers) {
          std::cout << "IP: " << listItem.ip << " Port: " << listItem.port << std::endl;
      }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "no file path given!" << std::endl;
    return -1;
  }

  torrentProperties torrentContent = decodeTorrent(std::string (argv[1]));
  std::string response = communicateToTracker(torrentContent);
  announceProperties announceContent = decodeAnnounceResponse(response);

  //printTorrentProperties(torrentContent);
  //printAnnounceProperties(announceContent);   // if you want to see how all properties get organized

  std::vector<std::thread> peerThreads(announceContent.peers.size());

  std::mutex queueMutex;
  for (int i= 0; i < announceContent.peers.size(); i++) {
    peerThreads[i] = std::thread(communicateWithPeers, std::ref(announceContent), std::ref(torrentContent), std::ref(announceContent.peers[i]), std::ref(queueMutex));
  }
  for (int j=0; j < announceContent.peers.size(); j++) {
    peerThreads[j].join();
  }

  if(torrentContent.fileBuiltPieces.size() != torrentContent.numOfPieces) {
    std::cout << "failed to download all pieces!" << std::endl;
    return -1;
  }
  std::cout << "Downloaded all pieces successfully" << std::endl;
  bool fileBuilt = buildFile(torrentContent.fileBuiltPieces, torrentContent.name);
  if (fileBuilt) {
    std::cout << "successfully built the file and put it in the 'downloaded_files' directory" << std::endl;
  }
  
};