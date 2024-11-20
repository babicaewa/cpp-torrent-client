#include "handshake/handshake.h"
#include <sys/socket.h>
#include <unistd.h>
#include "utils/utils.h"

/*
    Creates the handshake to send to a peer
*/

std::vector<unsigned char> createHandshake(torrentProperties& torrentContents) { 

    std::vector<unsigned char> handshake(68);
    std::string pstr = "BitTorrent protocol";

    handshake[0] = (unsigned char)0x13;
    memcpy(&handshake[1], pstr.c_str(), pstr.length());
    memset(&handshake[20], 0, 8);

    std::vector<unsigned char> infoHex = decodeHashStringtoPureHex(torrentContents.infoHash);
    std::vector<unsigned char> peerIDHex = decodeHashStringtoPureHex(torrentContents.peerID);

    memcpy(&handshake[28], infoHex.data(), 20);
    memcpy(&handshake[48], peerIDHex.data(), 20);

    return handshake;
}

/*
    Sends a handshake to a peer, and parses its response
*/

int sendPeerHandshake(int& clientSocket, std::vector<unsigned char> handshake, peer& peer) {
        std::vector<unsigned char> handshakeResponseBuffer(68);

    if(send(clientSocket, handshake.data(), 68, 0) == -1) {
        std::cout << "Could not send the handshake to: " << peer.ip << ":" << peer.port << "->" << strerror(errno) << std::endl;
        return -1;
    }
    while (true) {
        sleep(5);   //waiting for handshake back
        ssize_t bytesRead = read(clientSocket, handshakeResponseBuffer.data(), handshakeResponseBuffer.size());
        if (bytesRead == -1) {
             std::cout << "error receiving handshake from: " << peer.ip << std::endl;          //reads response from handshake
             return -1;
        }
        else if (bytesRead == 0) {
            std::cout << "no response for handshake from: " << peer.ip << std::endl;
            return -1;
        }
        break;
    }


    for (int i = 0; i < 20; i++) {
        if (handshakeResponseBuffer[i+28] != handshake[i+28]) {
            std::cout << "handshakes don't match!" << std::endl;    //checking if handshakes match
            return -1;
        }
    }
    std::cout << "successful handshake with: " << peer.ip << ":" << peer.port << std::endl;
    return 0;
}