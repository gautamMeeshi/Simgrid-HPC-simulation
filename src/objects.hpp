#include <vector>
#include "helper.hpp"

#ifndef OBJECTS_H
#define OBJECTS_H

enum SlurmSignal {
    RUN = 0,
    STOP = 1,
    IDLE = 2,
    SLEEP = 3
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
    double relinquish_time; // time when the resource will become free
    Resource(SlurmdState ns = FREE, int fs = 2, double rt = 0) {
        node_state = ns;
        free_cpus = fs;
        relinquish_time = rt;
    }
};

class JobLogs {
public:
    double start_time;
    double end_time;
    std::vector<int> nodes_running;
    JobLogs (double st = 0, double et = 0, std::vector<int> nr = std::vector<int>()) {
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