#include "peerComm/peerComm.h"
#include <vector>

int waitForUnchoke(int& cilentSocket, peerStatus& peerStatusInfo, peer& peer);

int sendInterestedMsg(int& clientSocket);

int sendHaveMsg(int& clientSocket, int&index);

std::vector<unsigned char> createRequestMsg(int piece, int beginIndex, int blockLength);