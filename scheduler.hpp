#include <map>
#include <vector>
#include "helper.hpp"
#include "objects.hpp"

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

std::vector<SlurmCtlDmsg*> fcfs_backfill_scheduler(std::vector<Job*> &jobs,
                                                   std::vector<Resource> &resrc,
                                                   long &jobs_remaining) {
    std::vector<SlurmCtlDmsg*> res;
    int total_free_cpus = 0;
    for (int j=0; j<resrc.size(); j++) {
        total_free_cpus += resrc[j].free_cpus;
        res.push_back(new SlurmCtlDmsg());
    }
    int i = 0;
    int j = 0;
    while (i < jobs.size()) {
        // find if a job can be scheduled
        if (jobs[i]->num_cpus <= total_free_cpus) {
            while (j<resrc.size()) { // iterate over the number of SlurmDs
                if (resrc[j].free_cpus > 0) {
                    int cpus_to_use = std::min(resrc[j].free_cpus,
                                               jobs[i]->num_cpus);
                    Job *job_subset = new Job(jobs[i]->job_id, cpus_to_use,
                                              jobs[i]->computation_cost,
                                              jobs[i]->priority,
                                              jobs[i]->p_job_id);
                    res[j]->jobs.push_back(job_subset);
                    res[j]->sig = RUN;
                    resrc[j].free_cpus -= cpus_to_use;
                    total_free_cpus -= cpus_to_use;
                    jobs[i]->num_cpus = jobs[i]->num_cpus - cpus_to_use;
                    if (jobs[i]->num_cpus == 0) {
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

std::vector<SlurmCtlDmsg*> fcfs_scheduler(std::vector<Job*> &jobs,
                                          std::vector<Resource> &resrc,
                                          long &jobs_remaining) {
std::vector<SlurmCtlDmsg*> res;
    int total_free_cpus = 0;
    for (int j=0; j<resrc.size(); j++) {
        total_free_cpus += resrc[j].free_cpus;
        res.push_back(new SlurmCtlDmsg());
    }
    int i = 0;
    int j = 0;
    bool job_scheduled = false;
    for (int i=0; i < jobs.size(); i++) {
        if (jobs[i]->num_cpus <= total_free_cpus) {
            while (j<resrc.size()) { // iterate over the number of SlurmDs
                if (resrc[j].free_cpus > 0) {
                    int cpus_to_use = std::min(resrc[j].free_cpus,
                                               jobs[i]->num_cpus);
                    Job *job_subset = new Job(jobs[i]->job_id, cpus_to_use,
                                              jobs[i]->computation_cost,
                                              jobs[i]->priority,
                                              jobs[i]->p_job_id);
                    res[j]->jobs.push_back(job_subset);
                    res[j]->sig = RUN;
                    resrc[j].free_cpus -= cpus_to_use;
                    total_free_cpus -= cpus_to_use;
                    jobs[i]->num_cpus = jobs[i]->num_cpus - cpus_to_use;
                    if (jobs[i]->num_cpus == 0) {
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

std::vector<SlurmCtlDmsg*> scheduler(std::map<int, Job*> &jobs,
                                     std::vector<Resource> &resrc,
                                     std::string scheduler_type,
                                     long &jobs_remaining) {
    std::vector<Job*> runnable_jobs = getRunnableJobs(jobs);
    std::vector<SlurmCtlDmsg*> res;
    if (scheduler_type == "fcfs_backfill") {
        res = fcfs_backfill_scheduler(runnable_jobs, resrc, jobs_remaining);
    } else {
        res = fcfs_scheduler(runnable_jobs, resrc, jobs_remaining);
    }
    return res;
}

#endif