#include <string>
#include <iostream>
#include "bencode/bdecode.h"

#ifndef TRACKERCOMM_H
#define TRACKERCOMM_H

std::string generatePeerID();

std::string createAnnounceQuery(torrentProperties& torrentContents);

std::string announceRequest(const std::string& query);
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

std::string communicateToTracker(torrentProperties& torrentContents);

#endif //TRACKER_COMM_H