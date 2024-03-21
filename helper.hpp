#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>

#ifndef HELPER_H // include guard
#define HELPER_H


enum JobState {
  PENDING = 0, // waiting for resources
  RUNNING = 1, // currently running
  COMPLETED = 2, // completed
  PREEMPTED = 3 // premepted and will be resumed later
};

class Job {
public:
  int job_id; // job_id of this job
  int num_cpus; // number of CPUs to be distributed on
  double computation_cost; // computation per cpu
  int priority; // higher number means higher priority
  std::vector<int> p_job_id; // jobs on which this job is dependent
  JobState job_state;
  Job(int ji = 0, int nc = 0, double cc = 0.0, int prty = 0,
      std::vector<int> prnt = std::vector<int>()) {
    job_id = ji;
    num_cpus = nc;
    computation_cost = cc;
    priority = prty;
    p_job_id = prnt;
    job_state = PENDING;
  }

  friend std::ostream & operator << (std::ostream &os, const Job &j) {
    os << "Job id = " << j.job_id <<'\n';
    os << "Num cpus = " << j.num_cpus << '\n';
    os << "Computation volume = " << j.computation_cost <<'\n';
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

Job* getJobFromStr(std::string line) {
  std::string str = "";
  int ji, nc, prty;
  double cc;
  std::vector<int> prnt;
  int count = 0;
  for (int i=0; i < line.size(); i++) {
    if (line[i] == ',' || line[i] == '(' || line[i] == ')') {
      if (count == 0){
        ji = stoi(str);
      } else if (count == 1) {
        nc = stoi(str);
      } else if (count == 2) {
        cc = stod(str);
      } else if (count == 3) {
        prty = stoi(str);
      } else {
        if (str != "") {
          int temp = stoi(str);
          prnt.push_back(temp);
        }
      }
      count++;
      str = "";
    } else {
      str+=line[i];
    }
  }
  Job *j = new Job(ji, nc, cc, prty, prnt);
  return j;
}

std::vector<Job*> parseJobFile(std::string job_file_name) {
    std::vector<Job*> jobs;
    std::ifstream inputFile(job_file_name);
    std::string line;
    bool skip = true;
    while (std::getline(inputFile, line)) {
      if (skip) {
        skip = false;
        continue;
      }
      Job *j = getJobFromStr(line);
      jobs.push_back(j);
    }
    // Close the file when done
    inputFile.close();
    return jobs;
}

#endif