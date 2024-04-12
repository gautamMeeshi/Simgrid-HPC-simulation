#include <vector>
#include "helper.hpp"

#ifndef OBJECTS_H
#define OBJECTS_H

const int CPUS_PER_NODE = 2;

enum SlurmSignal {
    RUN = 0,
    STOP = 1,
    IDLE = 2
};

enum SlurmdState {
    FREE = 0,
    BUSY = 1
};

class SlurmdMsg {
public:
    SlurmdState state;
    int free_cpus;
    std::vector<int> jobs_completed;
    SlurmdMsg (SlurmdState s, int fc, std::vector<int> jc) {
        free_cpus = fc;
        jobs_completed = jc;
        state = s;
    }
};

class SlurmCtldMsg {
public:
    SlurmSignal sig;
    std::vector<Job*> jobs;
    SlurmCtldMsg (SlurmSignal s = IDLE,
                  std::vector<Job*> j = std::vector<Job*>()) {
        sig = s;
        jobs = j;
    }
};

class Resource {
public:
    SlurmdState node_state;
    int free_cpus;
};

class JobLogs {
public:
    long start_time;
    long end_time;
    std::vector<int> nodes_running;
    JobLogs (long st = 0, long et = 0, std::vector<int> nr = std::vector<int>()) {
        start_time = st;
        end_time = et;
        nodes_running = nr;
    }
};

class InfoMsg {
public:
    std::vector<std::pair<double, double>> energy_log;
    InfoMsg (std::vector<std::pair<double, double>> e) {
        energy_log = e;
    } 
};

#endif