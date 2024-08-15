#include <map>
#include <vector>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <algorithm>

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
            root.put("jobs", convertJobs2Str(jobs));
            std::ostringstream oss;
            pt::write_json(oss, root);
            std::string json_string = oss.str();
            // send using the socket
            const char *cstr_ptr = json_string.c_str();
            send(sock,cstr_ptr, strlen(cstr_ptr), 0);
        }
    }

    ~Scheduler() {
        if (type.substr(0,6) == "remote") {
            pt::ptree root;
            root.put("state", "off");
            std::ostringstream oss;
            pt::write_json(oss, root);
            std::string json_string = oss.str();
            // send using the socket
            const char *cstr_ptr = json_string.c_str();
            send(sock,cstr_ptr, strlen(cstr_ptr), 0);
            int ret = close(sock);
            if (ret == 0) {
                std::cout << "socket closed gracefully\n";
            } else {
                std::cout<< "ERROR SOCKET CLOSE, ERROR CODE "<< ret<<'\n';
            }
        }
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

    std::vector<SlurmCtldMsg*> fcfs_backfill_scheduler(std::vector<Job*> &jobs,
                                                       std::vector<Resource> &resrc,
                                                       long &jobs_remaining,
                                                       double curr_time) {
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
        // reservation time is the time when the nodes required for the top
        // of the queue job will be available
        // NOTE - reservation time does not change if shorter jobs get scheduled in free nodes
        double reservation_time = 0;
        std::vector<double> relinquish_times;
        bool fcfs = true;

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
        int j = 0;
        while (i < jobs.size()) {
            // find if a job can be scheduled
            int nodes_distributed_on = 0;
            if (jobs[i]->nodes <= total_free_nodes && (fcfs || jobs[i]->run_time < reservation_time)) {
                while (j<resrc.size()) { // iterate over the number of SlurmDs
                    if (resrc[j].node_state == FREE) {
                        Job *job_subset = new Job(jobs[i]->job_id, 1,
                                                jobs[i]->tasks_per_node,
                                                jobs[i]->cpus_per_task,
                                                jobs[i]->computation_cost,
                                                jobs[i]->priority,
                                                jobs[i]->p_job_id);
                        res[j]->jobs.push_back(job_subset);
                        res[j]->sig = RUN;
                        resrc[j].free_cpus = 0;
                        resrc[j].node_state = BUSY;
                        resrc[j].relinquish_time = curr_time + job_subset->run_time + 10;
                        relinquish_times.push_back(resrc[j].relinquish_time);
                        total_free_nodes--;
                        nodes_distributed_on++;
                        if (nodes_distributed_on == jobs[i]->nodes) {
                            jobs[i]->job_state = RUNNING;
                            jobs_remaining--;
                            j++;
                            break;
                        }
                    }
                    j++;
                }
            } else if (fcfs) {
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
        curr_free_nodes = total_free_nodes;
        return res;
    }

    std::vector<SlurmCtldMsg*> aggressive_backfill_scheduler(std::vector<Job*> &jobs,
                                                       std::vector<Resource> &resrc,
                                                       long &jobs_remaining) {
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
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
        int j = 0;
        while (i < jobs.size()) {
            // find if a job can be scheduled
            int nodes_distributed_on = 0;
            if (jobs[i]->nodes <= total_free_nodes) {
                while (j<resrc.size()) { // iterate over the number of SlurmDs
                    if (resrc[j].node_state == FREE) {
                        Job *job_subset = new Job(jobs[i]->job_id, 1,
                                                jobs[i]->tasks_per_node,
                                                jobs[i]->cpus_per_task,
                                                jobs[i]->computation_cost,
                                                jobs[i]->priority,
                                                jobs[i]->p_job_id);
                        res[j]->jobs.push_back(job_subset);
                        res[j]->sig = RUN;
                        resrc[j].free_cpus = 0;
                        resrc[j].node_state = BUSY;
                        total_free_nodes--;
                        nodes_distributed_on++;
                        if (nodes_distributed_on == jobs[i]->nodes) {
                            jobs[i]->job_state = RUNNING;
                            jobs_remaining--;
                            j++;
                            break;
                        }
                    }
                    j++;
                }
            }
            i++;
        }
        return res;
    }

    std::vector<SlurmCtldMsg*> fcfs_scheduler(std::vector<Job*> &jobs,
                                              std::vector<Resource> &resrc,
                                              long &jobs_remaining) {
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
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
        int j = 0;
        for (int i=0; i < jobs.size(); i++) {
            int nodes_distributed_on = 0;
            if (jobs[i]->nodes <= total_free_nodes) {
                while (j<resrc.size()) { // iterate over the nodes
                    if (resrc[j].node_state == FREE) {
                        Job *job_subset = new Job(jobs[i]->job_id, 1,
                                                  jobs[i]->tasks_per_node,
                                                  jobs[i]->cpus_per_task,
                                                  jobs[i]->computation_cost,
                                                  jobs[i]->priority,
                                                  jobs[i]->p_job_id);
                        res[j]->jobs.push_back(job_subset);
                        res[j]->sig = RUN;
                        resrc[j].free_cpus = 0;
                        resrc[j].node_state = BUSY;
                        total_free_nodes--;
                        nodes_distributed_on++;
                        if (nodes_distributed_on == jobs[i]->nodes) {
                            jobs[i]->job_state = RUNNING;
                            jobs_remaining--;
                            j++;
                            break;
                        }
                    }
                    j++;
                }
            } else {
                break; // not allowed to skip the sequence in fcfs
            }
        }
        return res;
    }

    std::vector<SlurmCtldMsg*> remote_send_all_jobs(std::map<int, Job*> &jobs,
                                                    std::vector<Resource> &resrc,
                                                    long &jobs_remaining,
                                                    double curr_time) {
        // create json req string
        std::string free_nodes;
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
        int nodes_distributed_on = 0;
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
        std::vector<Job*> jobsv = getRunnableJobs(jobs);
        pt::ptree root;
        root.put("free_nodes", free_nodes);
        root.put("jobs", convertJobs2Str(jobs));
        root.put("state", "on");
        root.put("relinquish_times", getRelinquishTimes(resrc));
        root.put("curr_time", curr_time);
        std::ostringstream oss;
        pt::write_json(oss, root);
        std::string json_string = oss.str();
        // send using the socket
        const char *cstr_ptr = json_string.c_str();
        send(sock,cstr_ptr, strlen(cstr_ptr), 0);
        // receive the response
        char buffer[1024*16] = {0};
        int output_len = read(sock, buffer, 1024*16) - 3;
        // parse and distribute the response the response
        // std::vector<int> to_run = str2IntList(buffer, recv_len);
        assert(output_len == jobsv.size());
        int j = 0;
        for (int i=0; i < output_len; i++) {
            if (buffer[i+3] == '1' && jobsv[i]->nodes <= total_free_nodes) {
                // schedule the job
                nodes_distributed_on = 0;
                while (j<resrc.size()) { // iterate over the nodes
                    if (resrc[j].node_state == FREE) {
                        Job *job_subset = new Job(jobsv[i]->job_id, 1,
                                                  jobsv[i]->tasks_per_node,
                                                  jobsv[i]->cpus_per_task,
                                                  jobsv[i]->computation_cost,
                                                  jobsv[i]->priority,
                                                  jobsv[i]->p_job_id);
                        res[j]->jobs.push_back(job_subset);
                        res[j]->sig = RUN;
                        resrc[j].free_cpus = 0;
                        resrc[j].node_state = BUSY;
                        total_free_nodes--;
                        nodes_distributed_on++;
                        if (nodes_distributed_on == jobsv[i]->nodes) {
                            jobsv[i]->job_state = RUNNING;
                            jobs_remaining--;
                            j++;
                            break;
                        }
                    }
                    j++;
                }
            }
        }
        return res;
    }

    std::vector<SlurmCtldMsg*> remote_send_runnable_jobs(std::vector<Job*> &jobs,
                                                        std::vector<Resource> &resrc,
                                                        long &jobs_remaining,
                                                        double curr_time) {
        std::string free_nodes;
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
        int nodes_distributed_on = 0;
        if (counter == 5000) {
            train = true;
            counter = 0;
            std::cout<<"TRAIN = TRUE\n";
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
            // if there is no change in the number of free nodes
            // if no job can be scheduled then no point running the scheduling algo
            // skip scheduling algorithm
            bool skip = true;
            for (int i=0; i < jobs.size(); i++) {
                if (jobs[i]->nodes < total_free_nodes) {
                    skip = false;
                    break;
                }
            }
            if (skip) {
                return res;
            }
        }
        curr_free_nodes = total_free_nodes;
        pt::ptree root;
        root.put("free_nodes", free_nodes);
        root.put("jobs", convertJobs2Str(jobs));
        root.put("state", "on");
        root.put("relinquish_times", getRelinquishTimes(resrc));
        root.put("curr_time", curr_time);
        if (train) {
            root.put("train", "True");
            train = false;
        } else {
            root.put("train", "False");
        }
        std::ostringstream oss;
        pt::write_json(oss, root);
        std::string json_string = oss.str();
        // send using the socket
        const char *cstr_ptr = json_string.c_str();
        send(sock,cstr_ptr, strlen(cstr_ptr), 0);
        // receive the response
        char buffer[1024*16] = {0};
        int output_len = read(sock, buffer, 1024*16) - 3;
        // parse and distribute the response the response
        assert(output_len == jobs.size());
        int j = 0;
        for (int i=0; i < output_len; i++) {
            if (buffer[i+3] == '1' && jobs[i]->nodes <= total_free_nodes) {
                // schedule the job
                nodes_distributed_on = 0;
                while (j<resrc.size()) { // iterate over the nodes
                    if (resrc[j].node_state == FREE) {
                        Job *job_subset = new Job(jobs[i]->job_id, 1,
                                                  jobs[i]->tasks_per_node,
                                                  jobs[i]->cpus_per_task,
                                                  jobs[i]->computation_cost,
                                                  jobs[i]->priority,
                                                  jobs[i]->p_job_id);
                        res[j]->jobs.push_back(job_subset);
                        res[j]->sig = RUN;
                        resrc[j].free_cpus = 0;
                        resrc[j].node_state = BUSY;
                        total_free_nodes--;
                        nodes_distributed_on++;
                        if (nodes_distributed_on == jobs[i]->nodes) {
                            jobs[i]->job_state = RUNNING;
                            jobs_remaining--;
                            j++;
                            break;
                        }
                    }
                    j++;
                }
            }
        }
        return res;
    }

    std::vector<SlurmCtldMsg*> schedule(std::map<int, Job*> &jobs,
                                        std::vector<Resource> &resrc,
                                        long &jobs_remaining,
                                        double curr_time) {
        counter++;
        std::vector<Job*> runnable_jobs = getRunnableJobs(jobs);
        std::vector<SlurmCtldMsg*> res;
        if (type == "fcfs_backfill") {
            res = fcfs_backfill_scheduler(runnable_jobs, resrc, jobs_remaining, curr_time);
        } else if (type == "aggressive_backfill") {
            res = aggressive_backfill_scheduler(runnable_jobs, resrc, jobs_remaining);
        } else if (type == "remote_fcfs_bf" ||
                   type == "remote_heuristic") {
            res = remote_send_all_jobs(jobs, resrc, jobs_remaining, curr_time);
        } else if (type == "remote_nn" ||
                   type == "remote_qnn" ||
                   type == "remote_learn_nn" ||
                   type == "remote_nn3" ||
                   type == "remote_aggressive_bf") {
            res = remote_send_runnable_jobs(runnable_jobs, resrc, jobs_remaining, curr_time);
        } else {
            std::cout<<"Scheduler type did not match with any defaulting to fcfs";
            res = fcfs_scheduler(runnable_jobs, resrc, jobs_remaining);
        }
        return res;
    }
};
#endif