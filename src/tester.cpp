#include "helper.hpp"
#include <iostream>

int main(){
    // Job j = getJobFromStr("0,1,10000000,1,(-1)");
    // std::cout<<j;

    std::map<int, Job*> jobs = parseJobFile("jobs.csv");
    for (auto it = jobs.begin(); it != jobs.end(); it++) {
        Job j = *it->second;
        std::cout<<j;
        std::cout<<"-----------------\n";
    }
    return 0;
}