#include <map>
#include <vector>
#include "bencode/bdecode.h"

#ifndef HANDSHAKE_H
#define HANDSHAKE_H

std::vector<unsigned char> createHandshake(torrentProperties& torrentContents);
int sendPeerHandshake(int& clientSocket, std::vector<unsigned char> handshake, peer& peer);

#endif