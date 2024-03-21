#include "helper.hpp"
#include <iostream>

int main(){
    // Job j = getJobFromStr("0,1,10000000,1,(-1)");
    // std::cout<<j;

    std::vector<Job*> jobs = parseJobFile("jobs.csv");
    for (int i=0; i<jobs.size(); i++) {
        Job j = *jobs[i];
        std::cout<<j;
        std::cout<<"-----------------\n";
    }
    return 0;
}