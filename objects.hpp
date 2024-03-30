#include <vector>
#include "helper.hpp"

#ifndef OBJECTS_H
#define OBJECTS_H

enum SlurmSignal {
    RUN = 0,
    STOP = 1,
    IDLE = 2
};

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
    SlurmSignal sig;
    std::vector<Job*> jobs;
    SlurmCtlDmsg (SlurmSignal s = IDLE,
                  std::vector<Job*> j = std::vector<Job*>()) {
        sig = s;
        jobs = j;
    }
};

class Resource {
public:
    int free_cpus;
};

#endif