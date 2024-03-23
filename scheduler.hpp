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

std::vector<Job*> fcfs_backfill_scheduler(std::vector<Job*> &jobs,
                                          std::vector<Resource> &resrc,
                                          long &jobs_remaining) {
    std::vector<Job*> res;
    bool job_distributed = false;
    int total_free_cpus = 0;
    for (int j=0; j<resrc.size(); j++) {
        total_free_cpus += resrc[j].free_cpus;
    }
    for (int i=0; i<jobs.size(); i++) {
        //find if a job can be scheduled
        if (jobs[i]->num_cpus <= total_free_cpus) {
            for (int j=0; j<resrc.size(); j++) {
                if (resrc[j].free_cpus > 0 && !job_distributed) {
                    int cpus_to_use = std::min(resrc[j].free_cpus,
                                               jobs[i]->num_cpus);
                    Job *job_subset = new Job(jobs[i]->job_id, cpus_to_use,
                                              jobs[i]->computation_cost,
                                              jobs[i]->priority,
                                              jobs[i]->p_job_id);
                    res.push_back(job_subset);
                    resrc[j].free_cpus -= cpus_to_use;
                    if (cpus_to_use < jobs[i]->num_cpus) {
                        jobs[i]->num_cpus = jobs[i]->num_cpus - cpus_to_use;
                    } else {
                        jobs[i]->job_state = RUNNING;
                        jobs_remaining--;
                        job_distributed = true;
                    }
                } else {
                    res.push_back(new Job());
                }
            }
        }
        if (job_distributed) {
            break;
        }
    }
    if (!job_distributed){
        for (int i=0; i<resrc.size(); i++) {
            res.push_back(new Job());
        }
        return res;
    }
    return res;
}

std::vector<Job*> fcfs_scheduler(std::vector<Job*> &jobs,
                                 std::vector<Resource> &resrc,
                                 long &jobs_remaining) {
    std::vector<Job*> res;
    bool job_distributed = false;
    int total_free_cpus = 0;
    for (int j=0; j<resrc.size(); j++) {
        total_free_cpus += resrc[j].free_cpus;
    }
    if (jobs.size() == 0 || total_free_cpus < jobs[0]->num_cpus) {
        for (int i=0; i<resrc.size(); i++) {
            res.push_back(new Job());
        }
        return res;
    }
    for (int j=0; j<resrc.size(); j++) {
        if (resrc[j].free_cpus > 0 && !job_distributed) {
            int cpus_to_use = std::min(resrc[j].free_cpus, jobs[0]->num_cpus);
            Job *job_subset = new Job(jobs[0]->job_id, cpus_to_use,
                                      jobs[0]->computation_cost,
                                      jobs[0]->priority, jobs[0]->p_job_id);
            res.push_back(job_subset);
            if (cpus_to_use < jobs[0]->num_cpus) {
                jobs[0]->num_cpus = jobs[0]->num_cpus - cpus_to_use;
            } else {
                jobs[0]->job_state = RUNNING;
                jobs_remaining--;
                job_distributed = true;
            }
        } else {
            res.push_back(new Job());
        }
    }
    return res;
}

std::vector<Job*> scheduler(std::vector<Job*> &jobs,
                            std::vector<Resource> &resrc,
                            std::string scheduler_type,
                            long &jobs_remaining) {
    std::vector<Job*> runnable_jobs = getRunnableJobs(jobs);
    std::vector<Job*> res;
    if (scheduler_type == "fcfs_backfill") {
        res = fcfs_backfill_scheduler(runnable_jobs, resrc, jobs_remaining);
    } else {
        res = fcfs_scheduler(runnable_jobs, resrc, jobs_remaining);
    }
    return res;
}

#endif