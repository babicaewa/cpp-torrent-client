#include <vector>
#include "bencode/bdecode.h"
#include "peerComm/peerComm.h"

peerStatus readPeerBitfield(int& clientSocket, peer& peer);

bool peerHasPiece(std::vector<unsigned char>& bitfield, int& index);