#include <vector>
#include <iostream>
#include "constants.hpp"

#ifndef OBJECTS_H
#define OBJECTS_H

enum JobState {
  PENDING = 0, // waiting for resources
  RUNNING = 1, // currently running
  COMPLETED = 2, // completed
  PREEMPTED = 3 // premepted and will be resumed later
};

class Job {
public:
  int job_id; // job_id of this job
  int nodes; // number of nodes required
  int tasks_per_node; // number of tasks per node
  int cpus_per_task; // number of tasks per cpu
  double computation_cost; // computation per thread
  int priority; // higher number means higher priority
  std::vector<int> p_job_id; // jobs on which this job is dependent
  JobState job_state; // state of job
  double run_time; // scaled value of runtime used by the schedulers
  double start_time;
  double injection_time; // time at which the job is put into the scheduler
  Job(int ji = 0, int n = 0, int tpn = 0, int cpt = 0,
      double cc = 0.0, int prty = 0, double in_time = 0,
      std::vector<int> prnt = std::vector<int>()) {
    job_id = ji;
    nodes = n;
    tasks_per_node = tpn;
    cpus_per_task = cpt;
    computation_cost = cc;
    priority = prty;
    p_job_id = prnt;
    injection_time = in_time;
    job_state = PENDING;
    run_time = (double)(cc*tpn*cpt)/((double)CPU_FREQUENCY);
    int total_threads = cpt*tpn;
    if (total_threads <= CORES_PER_CPU*CPUS_PER_NODE) {
      run_time = run_time/(double)(total_threads);
    } else {
      run_time = run_time/(double)(CORES_PER_CPU*CPUS_PER_NODE);
    }
    start_time = -1;
  }

  friend std::ostream & operator << (std::ostream &os, const Job &j) {
    os << "Job id = " << j.job_id <<'\n';
    os << "Num nodes = " << j.nodes << '\n';
    os << "Num tasks per node = " << j.tasks_per_node << '\n';
    os << "Num cpus per task = " << j.cpus_per_task << '\n';
    os << "Computation volume per core = " << j.computation_cost <<'\n';
    os << "Priority = " << j.priority << '\n';
    os << "Job state = " << j.job_state <<'\n';
    os << "Parent jobs: ";
    //os << j.p_job_id.size();
    for (int i=0; i< j.p_job_id.size(); i++) {
      os<<j.p_job_id[i]<<',';
    }
    os<<'\n';
    return os;
  }
};

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
    double waiting_time = 0;
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