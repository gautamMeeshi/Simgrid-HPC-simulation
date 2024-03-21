#include "helper.hpp"
#include <unordered_map>
#include <vector>

#ifndef SCHEDULER_H
#define SCHEDULER_H
/*
This file needs to define the scheduler funtion

Input - list of Pending Jobs, Resource list
Remove the Job from the Jobs list for now

Resource list - currently the number of free cpus on each node 
Later can include many other resources

Output - Resource to Job mapping
Output is a dictionary - {Resource: Job_to_be_run}

The SlurmCtlD will run that mapping
*/

class Resource {
    /*
    Contains all the resource objects that are considered while scheduling
    Currently contains only the free_cpus list on each node
    */
public:
    int free_cpus;
};

bool allParentsCompleted(std::vector<Job*> &jobs, std::vector<int> &p_job_id) {
    for (int i=0; i<p_job_id.size(); i++) {
        if (p_job_id[i] >= 0) {
            for (int j=0; j<jobs.size(); j++) {
                if (jobs[j]->job_id == p_job_id[i] &&
                    jobs[j]->job_state != COMPLETED) {
                    return false;
                }
            }
        }
    }
    return true;
}

std::vector<Job*> getRunnableJobs(std::vector<Job*> &jobs) {
    std::vector<Job*> res;
    for (int i=0; i<jobs.size(); i++) {
        if (jobs[i]->job_state == PENDING &&
            allParentsCompleted(jobs, jobs[i]->p_job_id)) {
            res.push_back(jobs[i]);
        }
    }
    return res;
}

std::vector<Job*> backfill_scheduler(std::vector<Job*> &jobs,
                                     std::vector<Resource> &resrc) {
    std::vector<Job*> runnable_jobs = getRunnableJobs(jobs);
    std::vector<Job*> res;
    // TODO - complete the function
    return res;
}

std::vector<Job*> fcfs_scheduler(std::vector<Job*> &jobs, std::vector<Resource> &resrc) {
    std::vector<Job*> runnable_jobs = getRunnableJobs(jobs);
    std::vector<Job*> res;
    // Runnable jobs are those that have no parent job pending
    bool job_distributed = false;
    int total_free_cpus = 0;
    for (int j=0; j<resrc.size(); j++) {
        total_free_cpus += resrc[j].free_cpus;
    }
    for (int j=0; j<resrc.size(); j++) {
        if (resrc[j].free_cpus > 0 && !job_distributed && 
            total_free_cpus >= jobs[0]->num_cpus) {

            int cpus_to_use = std::min(resrc[j].free_cpus, jobs[0]->num_cpus);
            Job *job_subset = new Job(jobs[0]->job_id, cpus_to_use,
                                      jobs[0]->computation_cost,
                                      jobs[0]->priority, jobs[0]->p_job_id);
            res.push_back(job_subset);
            if (cpus_to_use < jobs[0]->num_cpus) {
                jobs[0]->num_cpus = jobs[0]->num_cpus - cpus_to_use;
            } else {
                jobs.erase(jobs.begin());
                job_distributed = true;
            }
        } else {
            res.push_back(new Job());
        }
    }
    // std::cout<<"Returning from the scheduler\n";
    return res;
}

std::vector<Job*> scheduler(std::vector<Job*> &jobs,
                            std::vector<Resource> &resrc,
                            std::string scheduler_type) {
    std::vector<Job*> res;
    if (scheduler_type == "backfill") {
        res = backfill_scheduler(jobs, resrc);
    } else {
        res = fcfs_scheduler(jobs, resrc);
    }
    return res;
}

#endif