#include <map>
#include <vector>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
public:
    Scheduler(std::string scheduler_type) {
        type = scheduler_type;
        if (scheduler_type == "neural_network") {
            // create sockets
            sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in serv_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(8080);
            if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
                printf("\nPANIC:Invalid address/ Address not supported \n");
            }
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\nPANIC:Connection Failed \n");
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
                                                      long &jobs_remaining) {
        std::vector<SlurmCtldMsg*> res;
        int total_free_nodes = 0;
        for (int j=0; j<resrc.size(); j++) {
            if (resrc[j].node_state == FREE){
                total_free_nodes ++;
            }
            res.push_back(new SlurmCtldMsg());
        }
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

    std::vector<SlurmCtldMsg*> neural_network_scheduler(std::vector<Job*> &jobs,
                                                        std::vector<Resource> &resrc,
                                                        long &jobs_remaining) {
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
        pt::ptree root;
        root.put("free_nodes", free_nodes);
        pt::ptree jobs_json;
        for (int i=0; i < jobs.size(); i++) {
            pt::ptree job;
            job.push_back(std::make_pair("", pt::ptree()));
            job.back().second.put_value(jobs[i]->job_id);
            job.push_back(std::make_pair("", pt::ptree()));
            job.back().second.put_value(jobs[i]->nodes);
            job.push_back(std::make_pair("", pt::ptree()));
            job.back().second.put_value(jobs[i]->tasks_per_node);
            job.push_back(std::make_pair("", pt::ptree()));
            job.back().second.put_value(jobs[i]->cpus_per_task);
            pt::ptree p_job_id;
            for (int d=0; d < jobs[i]->p_job_id.size(); d++) {
                p_job_id.push_back(std::make_pair("", pt::ptree()));
                p_job_id.back().second.put_value(jobs[i]->p_job_id[d]);
            }
            job.push_back(std::make_pair("", p_job_id));
            jobs_json.push_back(std::make_pair("", job));
        }
        root.add_child("jobs", jobs_json);
        std::ostringstream oss;
        pt::write_json(oss, root);
        std::string json_string = oss.str();
        // send using the socket
        const char *cstr_ptr = json_string.c_str();
        send(sock,cstr_ptr, strlen(cstr_ptr), 0);
        // receive the response
        char buffer[1024] = {0};
        int output_len = read(sock, buffer, 1024);
        // parse and distribute the response the response
        int j = 0;
        for (int i=0; i < output_len; i++) {
            if (buffer[i] == '1') {
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
                                        long &jobs_remaining) {
        std::vector<Job*> runnable_jobs = getRunnableJobs(jobs);
        std::vector<SlurmCtldMsg*> res;
        if (type == "fcfs_backfill") {
            res = fcfs_backfill_scheduler(runnable_jobs, resrc, jobs_remaining);
        } else if (type == "neural_network") {
            res = neural_network_scheduler(runnable_jobs, resrc, jobs_remaining);
        } else {
            res = fcfs_scheduler(runnable_jobs, resrc, jobs_remaining);
        }
        return res;
    }
};
#endif