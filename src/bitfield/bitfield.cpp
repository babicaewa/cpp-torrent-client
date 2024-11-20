#include "bitfield/bitfield.h"
#include <sys/socket.h>
#include <unistd.h>


/*
    Reads peer's bitfield and parses which pieces it has
*/

peerStatus readPeerBitfield(int& clientSocket, peer& peer) {

    peerStatus peerStatusInfo;
    peerStatusInfo.peer = peer;
    peerStatusInfo.choked = true;

    std::vector<unsigned char>bitfieldBuffer;

    bitfieldBuffer.resize(5);   //resizing to hold msg length and msg type

    if((read(clientSocket,bitfieldBuffer.data(),bitfieldBuffer.size())) <= 0) {
        std::cout << "error reading bitfield" << std::endl;
    }
    else {
        if(bitfieldBuffer[4] == 5) {
        peerStatusInfo.bitFieldLength = static_cast<uint32_t>(bitfieldBuffer[0]) << 24 |
        static_cast<uint32_t>(bitfieldBuffer[1]) << 16 |
        static_cast<uint32_t>(bitfieldBuffer[2]) << 8 |
        static_cast<uint32_t>(bitfieldBuffer[3]);

        peerStatusInfo.bitFieldLength -= 1; //to adjust for msg type byte

        if (peerStatusInfo.bitFieldLength == 0) {
            return peerStatusInfo;
        }

        bitfieldBuffer.resize(peerStatusInfo.bitFieldLength + 5);   //resizing to grab bitfield, (+5 in case peer sends unchoke with bitfield message to avoid duplication in future unchoke call)

        read(clientSocket,bitfieldBuffer.data(),bitfieldBuffer.size());
        peerStatusInfo.bitfield = std::vector<unsigned char>(bitfieldBuffer.begin(),bitfieldBuffer.end()-5);
    }
        }
    return peerStatusInfo;
}

/*
    Bitwise operation to see if peer has piece "index"
*/

bool peerHasPiece(std::vector<unsigned char>& bitfield, int& index) {
    int byteIndex = index / 8;
    int offset = index % 8;
    return bitfield[byteIndex] >> ((7-offset) & 1) != 0;
}