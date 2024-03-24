#include <vector>
#include "helper.hpp"

#ifndef OBJECTS_H
#define OBJECTS_H

class SlurmDmsg {
public:
    int free_cpus;
    std::vector<int> jobs_completed;
    SlurmDmsg(int fc, std::vector<int> jc) {
        free_cpus = fc;
        jobs_completed = jc;
    }
};

class SlurmCtlDmsg {
public:
    std::vector<Job> jobs;
};

class Resource {
public:
    int free_cpus;
};

#endif