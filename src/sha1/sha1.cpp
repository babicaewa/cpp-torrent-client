#include <openssl/sha.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

/*
    Calculates the SHA-1 hash of a string *TAKEN FROM INTERNET*
*/

std::string sha1Hash(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];

    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
    
};

