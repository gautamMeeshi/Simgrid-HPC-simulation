#include <map>
#include <vector>
#include "helper.hpp"
#include "objects.hpp"

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

bool allParentsCompleted(std::map<int, Job*> &jobs,const std::vector<int> &p_job_id) {
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

std::vector<SlurmCtldMsg*> scheduler(std::map<int, Job*> &jobs,
                                     std::vector<Resource> &resrc,
                                     std::string scheduler_type,
                                     long &jobs_remaining) {
    std::vector<Job*> runnable_jobs = getRunnableJobs(jobs);
    std::vector<SlurmCtldMsg*> res;
    if (scheduler_type == "fcfs_backfill") {
        res = fcfs_backfill_scheduler(runnable_jobs, resrc, jobs_remaining);
    } else {
        res = fcfs_scheduler(runnable_jobs, resrc, jobs_remaining);
    }
    return res;
}

#endif