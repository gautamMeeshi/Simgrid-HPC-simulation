#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <map>
#include <string>

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
  int nodes; // number of nodes required
  int tasks_per_node; // number of tasks per node
  int cpus_per_task; // number of tasks per cpu
  double computation_cost; // computation per core
  int priority; // higher number means higher priority
  std::vector<int> p_job_id; // jobs on which this job is dependent
  JobState job_state;
  Job(int ji = 0, int n = 0, int tpn = 0, int cpt = 0,
      double cc = 0.0, int prty = 0, std::vector<int> prnt = std::vector<int>()) {
    job_id = ji;
    nodes = n;
    tasks_per_node = tpn;
    cpus_per_task = cpt;
    computation_cost = cc;
    priority = prty;
    p_job_id = prnt;
    job_state = PENDING;
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

Job* getJobFromStr(std::string line) {
  std::string str = "";
  int ji, n, tpn, cpt, prty;
  double cc;
  std::vector<int> prnt;
  int count = 0;
  for (int i=0; i < line.size(); i++) {
    if (line[i] == ',' || line[i] == '(' || line[i] == ')') {
      if (count == 0){
        ji = stoi(str);
      } else if (count == 1) {
        n = stoi(str);
      } else if (count == 2) {
        tpn = stoi(str);
      } else if (count == 3) {
        cpt = stoi(str);
      } else if (count == 4) {
        cc = floor(stod(str)/(n*tpn*cpt));
      } else if (count == 5) {
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
  Job *j = new Job(ji, n, tpn, cpt, cc, prty, prnt);
  return j;
}

std::map<int, Job*> parseJobFile(std::string job_file_name) {
  std::map<int, Job*> jobs;
  std::ifstream inputFile(job_file_name);
  std::string line;
  bool skip = true;
  while (std::getline(inputFile, line)) {
    if (skip) {
      skip = false;
      continue;
    }
    Job *j = getJobFromStr(line);
    jobs[j->job_id] = j;
  }
  // Close the file when done
  inputFile.close();
  return jobs;
}

bool jobExists(std::vector<Job*> jobs, int job_id) {
  if (job_id < 0 ) {
    return true;
  }
  for (int i=0; i<jobs.size(); i++) {
    if (jobs[i]->job_id == job_id) {
      return true;
    }
  }
  return false;
}

std::vector<int> str2IntList(char *jobs_str, int len) {
  std::vector<int> res;
  std::string temp = "";
  for (int i=0; i < len; i++) {
    if (jobs_str[i] == ',') {
      res.push_back(stoi(temp));
      temp = "";
    } else {
      temp += jobs_str[i];
    }
  }
  if (temp!="") {
    res.push_back(stoi(temp));
  }
  return res;
}

std::string convertJobs2Str(std::map<int, Job*> job_map) {
  // input - job map (key: job_id, value: job_ptr)
  // output - string "[job_id,nodes,tasks_per_node,cpus_per_task,[p_job_ids]],[...],[...]...""
  std::string str = "[";
  for (auto it = job_map.begin(); it!=job_map.end(); it++) {
    str += "[";
    Job *job_ptr = it->second;
    str += std::to_string(job_ptr->job_id) + ",";
    str += std::to_string(job_ptr->job_state) + ",";
    str += std::to_string(job_ptr->nodes) + ",";
    str += std::to_string(job_ptr->tasks_per_node) + ",";
    str += std::to_string(job_ptr->cpus_per_task) + ",";
    str += std::to_string(job_ptr->computation_cost) + ",";
    str += "[";
    for (int i=0; i<job_ptr->p_job_id.size(); i++) {
      str += std::to_string(job_ptr->p_job_id[i]);
      if (i != job_ptr->p_job_id.size()-1) {
        str += ",";
      }
    }
    str+="]],";
  }
  str = str.substr(0, str.size()-1);
  str+="]";
  return str;
}

std::string convertJobs2Str(std::vector<Job*> job_vec) {
  // input - job map (key: job_id, value: job_ptr)
  // output - string "[job_id,nodes,tasks_per_node,cpus_per_task,[p_job_ids]],[...],[...]...""
  std::string str = "[";
  for (int i = 0; i < job_vec.size(); i++) {
    str += "[";
    Job *job_ptr = job_vec[i];
    str += std::to_string(job_ptr->job_id) + ",";
    str += std::to_string(job_ptr->job_state) + ",";
    str += std::to_string(job_ptr->nodes) + ",";
    str += std::to_string(job_ptr->tasks_per_node) + ",";
    str += std::to_string(job_ptr->cpus_per_task) + ",";
    str += std::to_string(job_ptr->computation_cost) + ",";
    str += "[";
    for (int i=0; i<job_ptr->p_job_id.size(); i++) {
      str += std::to_string(job_ptr->p_job_id[i]);
      if (i != job_ptr->p_job_id.size()-1) {
        str += ",";
      }
    }
    str+="]],";
  }
  if (str.size() > 1) {
    str = str.substr(0, str.size()-1);
  }
  str+="]";
  return str;
}

#endif