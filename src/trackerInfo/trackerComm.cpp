#include <string>
#include "trackerInfo/trackerComm.h"
#include "bencode/bdecode.h"
#include <cstdlib>
#include <curl/curl.h>
#include <utils/utils.h>

/*
    Takes announce response and puts it into a string *TAKEN FROM YOUTUBE VIDEO*
*/

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

/*
    Formulates the URL announce query to send to the tracker
*/

std::string createAnnounceQuery(torrentProperties& torrentContents) {

    std::string queryURL;

    std::string port = "6881";


    torrentContents.peerID = generatePeerID();

    queryURL = torrentContents.announce + "?info_hash=" + hashToURLEncoding(torrentContents.infoHash) + "&peer_id=" +
    hashToURLEncoding(torrentContents.peerID) + "&port=" + port + "&uploaded=0&downloaded=0&compact=1&left="+ std::to_string(torrentContents.length);

    return queryURL;
};

/*
    Sends the URL announce query to the tracker
*/

std::string announceRequest(const std::string& query) {
    //std::cout << "Query: " << query << std::endl;
    CURL* curl;
    CURLcode res;
    std::string response;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, query.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }
    return response;
}

/*
    Main function to grab tracker info
*/

std::string communicateToTracker(torrentProperties& torrentContents) {
    std::string query = createAnnounceQuery(torrentContents);
    std::string trackerResponse = announceRequest(query);

    return trackerResponse;
};



