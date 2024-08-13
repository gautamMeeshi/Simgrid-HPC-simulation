#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <map>
#include <string>
#include "objects.hpp"
#include "constants.hpp"

#ifndef HELPER_H // include guard
#define HELPER_H

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
    str += std::to_string(job_ptr->run_time) + ",";
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
    str += std::to_string(job_ptr->run_time) + ",";
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

std::string getRelinquishTimes(std::vector<Resource> &resrc) {
  std::string res = "[";
  for (int i=0; i < resrc.size(); i++) {
    res += std::to_string(resrc[i].relinquish_time);
    if (i != resrc.size() - 1) {
      res += ",";
    }
  }
  res += "]";
  return res;
}

#endif