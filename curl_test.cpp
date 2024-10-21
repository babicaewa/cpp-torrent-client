#include <curl/curl.h>
#include <iostream>
#include <string>

// Write callback function for handling HTTP response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Simple function to perform HTTP GET request using libcurl
std::string announceRequest(const std::string& query) {
    CURL* curl;
    CURLcode res;
    std::string response;

    // Initialize curl globally
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

    curl_global_cleanup();
    return response;
}

int main() {
    std::string url = "http://www.example.com";
    std::string result = announceRequest(url);

    std::cout << "HTTP Response:\n" << result << std::endl;
    return 0;
}

announce-listll35:https://torrent.ubuntu.com/announceel40:https://ipv6.torrent.ubuntu.com/announceee
