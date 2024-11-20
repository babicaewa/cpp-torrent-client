#include "peerComm/messages.h"
#include <unistd.h>
#include <sys/socket.h>

/*
    Loop that checks if unchoked, timesout after 10 seconds of choking
*/

int waitForUnchoke(int& cilentSocket, peerStatus& peerStatusInfo, peer& peer) {
    std::vector<unsigned char> unchokeBuffer(5);

    ssize_t unchokeResponse;
    int counter = 0;
    while(counter < 10) {
        unchokeResponse = read(cilentSocket, unchokeBuffer.data(), unchokeBuffer.size());

        if (unchokeResponse == 0) {
            counter++;
            sleep(1);
        }
        else if (unchokeResponse == -1) {
            std::cout << "Error getting unchoked from: " << peer.ip << ":" << peer.port << std::endl;
            return -1;
        }
        else {
            if(unchokeBuffer[4] == 1) {
                peerStatusInfo.choked = false;
                return 0;
            }    //got unchoked
        }
    }
    return -1;
}

/*
    Sends an "interested" message to a peer
*/

int sendInterestedMsg(int& clientSocket) {
    std::vector<unsigned char> interestedMsg(5);
    interestedMsg[0] = 0;
    interestedMsg[1] = 0;
    interestedMsg[2] = 0;
    interestedMsg[3] = 1;
    
    interestedMsg[4] = 2;

    if(send(clientSocket, interestedMsg.data(), interestedMsg.size(), 0) == -1) {
        perror("Could not send interested message");
        return -1;
    }
    return 0;
}

/*
    Sends an "have" message to a peer
*/

int sendHaveMsg(int& clientSocket, int&index) {
    std::vector<unsigned char> haveMsg(9,0);
    haveMsg[3] = 5;
    haveMsg[4] = 4;
    haveMsg[5] = (index >> 24) & 0xFF;
    haveMsg[6] = (index >> 16) & 0xFF;
    haveMsg[7] = (index >> 8) & 0xFF;
    haveMsg[8] = index & 0xFF;


    if(send(clientSocket, haveMsg.data(), haveMsg.size(), 0) == -1) {
        perror("Could not send have message");
        return -1;
    }
    return 0;

}

/*
    Creates a "request" message for a block of a piece
*/

std::vector<unsigned char> createRequestMsg(int piece, int beginIndex, int blockLength) {
    std::vector<unsigned char> requestMsg(17,0);

    requestMsg[0] = 0;
    requestMsg[1] = 0;  //setting the length of the msg
    requestMsg[2] = 0;
    requestMsg[3] = 13;

    requestMsg[4] = 6;

    requestMsg[5] = (piece >> 24) & 0xFF;
    requestMsg[6] = (piece >> 16) & 0xFF;  //setting the piece index of the msg
    requestMsg[7] = (piece >> 8) & 0xFF;
    requestMsg[8] = (piece) & 0xFF;

    requestMsg[9] = (beginIndex >> 24) & 0xFF;
    requestMsg[10] = (beginIndex >> 16) & 0xFF;  //setting the starting index of the block
    requestMsg[11] = (beginIndex >> 8) & 0xFF;
    requestMsg[12] = (beginIndex) & 0xFF;

    requestMsg[13] = (blockLength >> 24) & 0xFF;
    requestMsg[14] = (blockLength >> 16) & 0xFF;  //setting the length of the block
    requestMsg[15] = (blockLength >> 8) & 0xFF;
    requestMsg[16] = (blockLength) & 0xFF;

    return requestMsg;

}

