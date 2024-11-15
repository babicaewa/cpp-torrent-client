#include "utils/utils.h"
#include <iomanip>
#include <iostream>

void displayProgressBar(int currVal, int totalVal) {
    float progressPercentage = static_cast<float>(currVal) / static_cast<float>(totalVal) * 100;
    int progressBarLength = 100;

    for (int i=0; i < progressBarLength; i++) {
        (i > progressPercentage) ? std::cout << " " : std::cout << "-";
    }
    std::cout << "] " << std::setprecision(4) << progressPercentage << "%\r" << std::flush;

    return;

}     