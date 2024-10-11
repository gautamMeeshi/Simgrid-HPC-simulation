#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <map>
#include <string>
#include <queue>
#include <cassert>
#include "objects.hpp"
#include "constants.hpp"

#ifndef HELPER_H // include guard
#define HELPER_H

Job* getJobFromStr(std::string line) {
  std::string str = "";
  int ji, n, tpn, cpt, prty;
  double cc;
  double in_time;
  std::vector<int> prnt;
  int count = 0;
  for (int i=0; i < line.size(); i++) {
    if (line[i] == ',' || line[i] == '(' || line[i] == ')') {
      if (count == 0){ // job index
        ji = stoi(str);
      } else if (count == 1) { // num nodes
        n = stoi(str);
      } else if (count == 2) { // tasks per node
        tpn = stoi(str);
      } else if (count == 3) { // cpu per task
        cpt = stoi(str);
      } else if (count == 4) { // computation volume per thread
        cc = floor(stod(str)/(n*tpn*cpt));
      } else if (count == 5) { // priority
        prty = stoi(str);
      } else if (count == 6) { // injection time
        in_time = stoi(str);
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
  Job *j = new Job(ji, n, tpn, cpt, cc, prty, in_time, prnt);
  return j;
}

std::deque<Job*> parseJobFile(std::string job_file_name) {
  std::deque<Job*> jobs;
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
    str += std::to_string(job_ptr->injection_time) + ",";
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
    str += std::to_string(job_ptr->injection_time) + ",";
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

// Input - vector of vector containing node operations with time_stamp
// Return - average resource utilization across all the nodes
// Stores the resource utilization of each node in /output/resource_utilization.log
double storeResourceUtlizationStats(std::vector<std::vector<std::pair<double, SlurmSignal>>> node_op_log,
                                    double total_run_time) {
  std::vector<double> total_active_time;
  int num_nodes = node_op_log.size();
  assert(num_nodes == NODES);
  for (int i=0; i < num_nodes; i++) {
    total_active_time.push_back(0);
    double start_time = -1;
    for (int j=0; j < node_op_log[i].size(); j++) {
      double time_stamp = node_op_log[i][j].first;
      if (node_op_log[i][j].second == RUN) {
        if (start_time == -1) {
          start_time = time_stamp;
        }
      } else if (node_op_log[i][j].second == SLEEP) {
        if (start_time != -1) {
          total_active_time[i] += time_stamp - start_time;
        } else {
          std::cout << "helper.hpp::WARNING: got a sleep time before start time at node "<<i <<'\n';
        }
        start_time = -1;
      } else if (node_op_log[i][j].second == STOP) {
        if (start_time != -1) {
          total_active_time[i] += time_stamp - start_time;
        }
        start_time = -1;
      }
    }
  }
  std::ofstream stats_file("output/resrc_util_stats.csv");
  stats_file << "node_number,resource_utilization\n";
  double avg_resrc_util = 0.0;
  for (int i=0; i < num_nodes; i++) {
    stats_file << i+1 << ',' << total_active_time[i]/total_run_time << '\n';
    avg_resrc_util += total_active_time[i]/total_run_time;
  }
  avg_resrc_util = avg_resrc_util/num_nodes;
  stats_file << "average_resource_utilization," << avg_resrc_util << '\n';
  stats_file.close();
  return avg_resrc_util;
}
#endif