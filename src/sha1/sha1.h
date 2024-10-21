#include <openssl/sha.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>


#ifndef SHA1_H
#define SHA1_H

std::string sha1Hash(const std::string& input);

#endif