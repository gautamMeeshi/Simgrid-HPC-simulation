#include <map>
#include <vector>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <sys/socket.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <algorithm>
#include <fstream>

#include "helper.hpp"
#include "objects.hpp"

namespace pt = boost::property_tree;

#ifndef SCHEDULER_H
#define SCHEDULER_H
/*
This file needs to define the scheduler funtion

Input - list of Pending Jobs, Resource list

Resource list - currently the number of free cpus on each node
Later can include many other resources

Output - vector that denotes the jobs to be run on each of the SlurmDs in sequence

The SlurmCtlD distributes the vector elements to the SlurmD
*/
class Scheduler {
private:
    std::string type;
    int sock;
    int curr_free_nodes;
    int counter;
    bool train = false;
    std::map<int, double> cumulative_runtime;
    std::map<int, Job*> pending_jobs;
    std::ofstream log_file;
public:
    Scheduler(std::string scheduler_type, std::map<int, Job*> &jobs) {
        type = scheduler_type;
        counter = 0;
        curr_free_nodes = 0;
        if (type.substr(0,6) == "remote") {
            // create sockets
            sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in serv_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(8080);
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
                printf("\nPANIC:Invalid address/ Address not supported \n");
            }
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\nPANIC:Connection Failed \n");
            }
            pt::ptree root;
            root.put("state", "on");
            root.put("scheduler_type", type);
            root.put("operation", "init");
            sendPtree(root);
        }
        log_file.open("output/run_log.csv");
    }

    ~Scheduler() {
        if (type.substr(0,6) == "remote") {
            pt::ptree root;
            root.put("state", "off");
            root.put("operation", "close");
            std::ostringstream oss;
            pt::write_json(oss, root);
            std::string json_string = oss.str();
            // send using the socket
            const char *cstr_ptr = json_string.c_str();
            send(sock, cstr_ptr, strlen(cstr_ptr), 0);
            int ret = close(sock);
            log_file.close();
            if (ret == 0) {
                std::cout << "socket closed gracefully\n";
            } else {
                std::cout<< "ERROR SOCKET CLOSE, ERROR CODE "<< ret<<'\n';
            }
        }
    }

    void sendPtree(pt::ptree root) {
        std::ostringstream oss;
        pt::write_json(oss, root);
        std::string json_string = oss.str();
        // send using the socket
        const char *cstr_ptr = json_string.c_str();
        send(sock,cstr_ptr, strlen(cstr_ptr), 0);
    }

    void writeRunLog(std::string &input, std::string &output) {
        // Input - input given to the Python NN model, output from the python NN model
        // Output - None
        // Operation - writes the input and output to the log file
        while(output.size() < 64) {
            output+="0";
        }
        output = output.substr(0,64);
        log_file << input << '|' << output << '\n';
        return;
    }

    void UpdateCumulativeRuntime(int job_id, double val) {
        cumulative_runtime[job_id] += val/8;
        for (int pid: pending_jobs[job_id]->p_job_id) {
            // if the runtime is more then child jobs should have less effect on the parent
            // following is a function that decreases aymptotically to 0 as run_time increases
            double weight = 1/pow((1+sqrt(pending_jobs[pid]->run_time)), 2);
            UpdateCumulativeRuntime(pid, weight*val);
        }
    }

    void injectJob(Job *job) {
        // Input job put in scheduler
        // Output - None
        // Operation - Updates any counter or graph structure required by the scheduler
        // Update cumulative_runtime
        cumulative_runtime[job->job_id] = 0;
        pending_jobs[job->job_id] = job;
        UpdateCumulativeRuntime(job->job_id, 1);
        
    }

    std::string getNNInput(std::vector<Job*> jobs, std::vector<Resource> &resrc, double curr_time) {
    //  Input - list of runnable jobs to be scheduled, resource information
    //  Output - float string to be sent to the Python process as input to Neural network
        std::string res;
        double max_runtime = 0;
        double max_computation = 0;
        
        for (int j=0; j < jobs.size(); j++) {
            if (jobs[j]->run_time > max_runtime) {
                max_runtime = jobs[j]->run_time;
            }
            if (jobs[j]->computation_cost > max_computation) {
                max_computation = jobs[j]->computation_cost;
            }
        }
        for (int i=0; i < resrc.size(); i++) {
            if (resrc[i].node_state == FREE) {
                res+="1,";
            } else {
                res+="0,";
            }
            if (resrc[i].relinquish_time > curr_time && max_runtime > 0) {
                res += std::to_string(std::round(10000*(resrc[i].relinquish_time - curr_time)/MAX_RUN_TIME)/10000)+",";
            } else {
                res += "0,";
            }
        }
        res = res.substr(0, res.size()-1);
        for (int i=0; i < NN_JOB_INPUT_SIZE; i++) {
            if (i < jobs.size()) {
                // the doubles are rounded to 4 decimals
                double nodes = std::round(10000*(double)jobs[i]->nodes/(double)NODES)/10000;
                double computation = std::round(10000*jobs[i]->computation_cost/max_computation)/10000;
                double runtime = std::round(10000*jobs[i]->run_time/max_runtime)/10000;
                res += "," + std::to_string(nodes) +
                       "," + std::to_string(computation) +
                       "," + std::to_string(cumulative_runtime[jobs[i]->job_id]) +
                       "," + std::to_string(runtime);
            } else {
                res += ",0,0,0,0";
            }
        }
        return res;
    }

    std::string parseNNOutput(const char *buffer, int len,
                              std::vector<Job*> &jobs,
                              int num_free_nodes) {
        // Input - byte stream from Python process, length of byte stream
        // Output - string in the form of 0,1 indicating to the Job or not
        // std::cout << "Jobs size " << jobs.size() << '\n';
        std::vector<std::pair<int, double>> result;
        int index = 0;
        std::string currentNumber;
        char run[64] = {};
        for (int i=0; i < 64; i++) {
            run[i] = '0';
        }

        for (int i = 0; i < len; i++) {
            if (buffer[i] == ',') {
                if (!currentNumber.empty()) {
                    result.push_back({index, std::stod(currentNumber)});
                    currentNumber.clear();
                    index++;
                }
            } else {
                currentNumber += buffer[i];
            }
            if (index == jobs.size()) {
                break;
            }
        }
        // Add the last number to the result if not empty
        if (!currentNumber.empty() && index < jobs.size()) {
            result.push_back({index, std::stod(currentNumber)});
            index++;
        }
        // std::cout << "Result from NN ";
        // for (int i=0; i<result.size(); i++) {
        //     std::cout << result[i].first<<','<<result[i].second << '|';
        // }
        // std::cout << '\n';
        // Sort based on the second value (double) in descending order
        stable_sort(result.begin(), result.end(),
                    [](const std::pair<int, double> &a, const std::pair<int, double> &b) {
                    return a.second > b.second;});
        for (std::pair<int, double> val: result) {
            if (jobs[val.first]->nodes <= num_free_nodes) {
                run[val.first] = '1';
                num_free_nodes -= jobs[val.first]->nodes;
            }
        }
        return std::string(run, 64);
    }

    bool allParentsCompleted(std::map<int, Job*> &jobs,
                             const std::vector<int> &p_job_id) {
        for (int i=0; i<p_job_id.size(); i++) {
            if (jobs[p_job_id[i]]->job_state != COMPLETED) {
                return false;
            }
        }
        return true;
    }

    std::vector<Job*> getRunnableJobs(std::map<int, Job*> &jobs) {
        std::vector<Job*> res;
        for (auto it=jobs.begin(); it!=jobs.end(); it++) {
            if (it->second->job_state == PENDING &&
                allParentsCompleted(jobs, it->second->p_job_id)) {
                res.push_back(it->second);
            }
        }
        return res;
    }

    int assignJob2Nodes(Job *job, std::vector<Resource> &resrc,
                        std::vector<SlurmCtldMsg*> &res, int node_idx,
                        int curr_time, std::vector<double> relinquish_times = std::vector<double>(),
                        bool fill_vector = false) {
        int nodes_distributed_on = 0;
        while (node_idx < resrc.size()) { // iterate over the nodes
            if (resrc[node_idx].node_state == FREE) {
                Job *job_subset = new Job(job->job_id, 1,
                                          job->tasks_per_node,
                                          job->cpus_per_task,
                                          job->computation_cost,
                                          job->priority,
                                          job->injection_time,
                                          job->p_job_id);
                res[node_idx]->jobs.push_back(job_subset);
                res[node_idx]->sig = RUN;
                resrc[node_idx].free_cpus = 0;
                resrc[node_idx].node_state = BUSY;
                resrc[node_idx].relinquish_time = curr_time + job_subset->run_time + 10;
                nodes_distributed_on++;
                if (fill_vector) {
                    relinquish_times.push_back(resrc[node_idx].relinquish_time);
                }
                if (nodes_distributed_on == job->nodes) {
                    job->job_state = RUNNING;
                    node_idx++;
                    break;
                }
            }
            node_idx++;
        }
        return node_idx;
    }

    std::vector<SlurmCtldMsg*> easy_backfill_scheduler(std::vector<Job*> &jobs,
                                                       std::vector<Resource> &resrc,
                                                       double curr_time) {
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
        // reservation time is the time when the nodes required for the top
        // of the queue job will be available
        // NOTE - reservation time does not change if shorter jobs get scheduled in free nodes
        double reservation_time = 0;
        std::vector<double> relinquish_times;
        bool fcfs = true;
        std::string nn_input = getNNInput(jobs, resrc, curr_time);
        std::string nn_output = "";

        for (int j=0; j<resrc.size(); j++) {
            if (resrc[j].node_state == FREE){
                total_free_nodes ++;
            } else {
                relinquish_times.push_back(resrc[j].relinquish_time);
            }
            res.push_back(new SlurmCtldMsg());
        }
        if (total_free_nodes == curr_free_nodes) {
            // if the number of free nodes has not changed then no need of scheduling
            return res;
        }
        int i = 0;
        int node_idx = 0;
        while (i < jobs.size()) {
            // find if a job can be scheduled
            int nodes_distributed_on = 0;
            if (jobs[i]->nodes <= total_free_nodes && (fcfs || jobs[i]->run_time < reservation_time)) {
                nn_output += '1';
                node_idx = assignJob2Nodes(jobs[i], resrc, res, node_idx, curr_time, relinquish_times, true);
                total_free_nodes -= jobs[i]->nodes;
            } else if (fcfs) {
                nn_output += "0";
                fcfs = false;
                std::sort(relinquish_times.begin(), relinquish_times.end());
                int reserved_nodes = jobs[i]->nodes - total_free_nodes;
                assert(reserved_nodes <= relinquish_times.size());
                    // PROOF -
                    // relinquish_times.size() = busy_nodes
                    // reserved_nodes = job_nodes - free_nodes
                    // job_nodes <= total_nodes
                    // job_nodes <= free_nodes + busy_nodes
                    // job_nodes - free_nodes <= busy_nodes
                    // reserved_nodes <= relinquish_times.size()
                reservation_time = relinquish_times[reserved_nodes-1];
            }
            i++;
        }
        writeRunLog(nn_input, nn_output);
        curr_free_nodes = total_free_nodes;
        return res;
    }

    std::vector<SlurmCtldMsg*> naive_backfill_scheduler(std::vector<Job*> &jobs,
                                                             std::vector<Resource> &resrc,
                                                             double curr_time) {
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
        std::string nn_input = getNNInput(jobs, resrc, curr_time);
        std::string nn_output = "";
        for (int j=0; j<resrc.size(); j++) {
            if (resrc[j].node_state == FREE){
                total_free_nodes ++;
            }
            res.push_back(new SlurmCtldMsg());
        }
        if (total_free_nodes == curr_free_nodes) {
            return res;
        }
        curr_free_nodes = total_free_nodes;
        int i = 0;
        int node_idx = 0;
        while (i < jobs.size()) {
            // find if a job can be scheduled
            int nodes_distributed_on = 0;
            if (jobs[i]->nodes <= total_free_nodes) {
                nn_output += "1";
                node_idx = assignJob2Nodes(jobs[i], resrc, res, node_idx, curr_time);
                total_free_nodes -= jobs[i]->nodes;
            }
            i++;
        }
        writeRunLog(nn_input, nn_output);
        return res;
    }

    std::vector<SlurmCtldMsg*> fcfs_scheduler(std::vector<Job*> &jobs,
                                              std::vector<Resource> &resrc,
                                              double curr_time) {
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
        std::string nn_input = getNNInput(jobs, resrc, curr_time);
        std::string nn_output = "";
        for (int j=0; j<resrc.size(); j++) {
            if (resrc[j].node_state == FREE) {
                total_free_nodes ++;
            }
            res.push_back(new SlurmCtldMsg());
        }
        if (total_free_nodes == curr_free_nodes) {
            return res;
        }
        curr_free_nodes = total_free_nodes;
        int node_idx = 0;
        for (int i=0; i < jobs.size(); i++) {
            if (jobs[i]->nodes <= total_free_nodes) {
                nn_output += "1";
                node_idx = assignJob2Nodes(jobs[i], resrc, res, node_idx, curr_time);
                total_free_nodes -= jobs[i]->nodes;
            } else {
                break; // not allowed to skip the sequence in fcfs
            }
        }
        writeRunLog(nn_input, nn_output);
        return res;
    }

    std::vector<SlurmCtldMsg*> remote_nn_scheduler(std::vector<Job*> jobs,
                                                   std::vector<Resource> &resrc,
                                                   double curr_time) {
        std::string free_nodes;
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
        int nodes_distributed_on = 0;
        char output[64];
        for (int i=0; i<64; i++) {
            output[i] = '0';
        }
        for (int i=0; i < resrc.size(); i++) {
            if (resrc[i].node_state == FREE) {
                free_nodes += "1";
                total_free_nodes++;
            } else {
                free_nodes += "0";
            }
            res.push_back(new SlurmCtldMsg());
        }
        if (total_free_nodes == curr_free_nodes) {
            return res;
        }
        curr_free_nodes = total_free_nodes;
        
        std::string nn_input = getNNInput(jobs, resrc, curr_time);
        pt::ptree root;
        root.put("nn_input", nn_input);
        root.put("state", "on");
        root.put("operation", "schedule");
        sendPtree(root);

        //recv output from NN model
        char buffer[1024] = {0};
        int output_len = read(sock, buffer, 1024);

        std::string nn_output = parseNNOutput(buffer, output_len, jobs, total_free_nodes);
        int node_idx=0;
        bool schedule = true;
        output_len = (jobs.size() < 64) ? jobs.size() : 64;
        for (int i=0; i < output_len; i++) {
            if (nn_output[i] == '1' && jobs[i]->nodes <= total_free_nodes) {
                output[i] = '1';
                node_idx = assignJob2Nodes(jobs[i], resrc, res, node_idx, curr_time);
                total_free_nodes -= jobs[i]->nodes;
            }
        }
        // second pass to assign the left over nodes in FCFS order
        for (int i=0; i<jobs.size(); i++) {
            if (jobs[i]->nodes <= total_free_nodes && jobs[i]->job_state == PENDING) {
                output[i] = '1';
                node_idx = assignJob2Nodes(jobs[i], resrc, res, node_idx, curr_time);
                total_free_nodes -= jobs[i]->nodes;
            }
        }
        std::string output_str = std::string(output, 64);
        writeRunLog(nn_input, output_str);
        return res;
    }

    std::vector<SlurmCtldMsg*> schedule(std::map<int, Job*> &jobs,
                                        std::vector<Resource> &resrc,
                                        double curr_time) {
        counter++;
        std::vector<Job*> runnable_jobs = getRunnableJobs(jobs);
        std::vector<SlurmCtldMsg*> res;
        if (type == "easy_backfill") {
            res = easy_backfill_scheduler(runnable_jobs, resrc, curr_time);
        } else if (type == "naive_backfill") {
            res = naive_backfill_scheduler(runnable_jobs, resrc, curr_time);
        } else if (type == "remote_nn") {
            res = remote_nn_scheduler(runnable_jobs, resrc, curr_time);
        } else if (type == "fcfs") {
            res = fcfs_scheduler(runnable_jobs, resrc, curr_time);
        } else {
            std::cout<<"Scheduler type did not match with any defaulting to fcfs";
        }
        return res;
    }
};
#endif