#include "helper.hpp"
#include <iostream>

int main(){
    // Job j = getJobFromStr("0,1,10000000,1,(-1)");
    // std::cout<<j;

    std::vector<Job> jobs = parseJobFile("jobs.csv");
    for (Job &j: jobs) {
        std::cout<<j;
        std::cout<<"-----------------\n";
    }
    return 0;
}