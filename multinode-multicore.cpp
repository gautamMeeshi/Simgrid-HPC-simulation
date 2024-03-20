#include <simgrid/s4u.hpp>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>
#include "helper.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

bool jobExists(std::vector<Job> &jobs, int job_id) {
  if (job_id < 0 ) {
    return true;
  }
  for (int i=0; i<jobs.size(); i++) {
    if (jobs[i].job_id == job_id) {
      return true;
    }
  }
  return false;
}

class SlurmCtlD {
  /*
  Collects the information from the SlurmDs
  Distributes the queue of work to SlurmD cores
  */

  std::string job_file_name      = "";
  long communicate_cost = 100;
  std::vector<sg4::Mailbox*> SlurmDs;
  sg4::Mailbox *mymailbox;
  std::vector<Job> jobs;
  std::vector<int> free_cpus;
  std::vector<std::string> host_name;

public:
  explicit SlurmCtlD(std::vector<std::string> args)
  {
    // xbt_assert(args.size() > 4, "The master function expects 3 arguments plus the workers' names");

    XBT_INFO("SlurmCltD args size %zu", args.size());
    job_file_name = args[1];
    mymailbox = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());
    jobs = parseJobFile(job_file_name);

    for (unsigned int i = 2; i < args.size(); i++) {
      SlurmDs.push_back(sg4::Mailbox::by_name(args[i]));
      free_cpus.push_back(0);
      host_name.push_back(args[i]);
    }

    XBT_INFO("Got %zu slurmds", SlurmDs.size());
  }

  int getFreeCpus() {
    int free_cpu_sum = 0;
    for (int i=0; i < SlurmDs.size(); i++) {
      auto msg = SlurmDs[i]->get<int>();
      free_cpus[i] = *msg;
      free_cpu_sum += free_cpus[i];
    }
    return free_cpu_sum;
  }

  void removeJobsWoParents(std::vector<Job> &jobs) {
    std::vector<int> jobs_wo_parents;
    do {
      jobs_wo_parents.clear();
      for (int i=jobs.size() - 1; i >= 0; i--) {
        for (int j=0; j < jobs[i].p_job_id.size(); j++) {
          if (!jobExists(jobs, jobs[i].p_job_id[j])) {
            jobs_wo_parents.push_back(i);
            break;
          }
        }
      }
      for (int i=0; i<jobs_wo_parents.size(); i++) {
        XBT_INFO("Removing job %i, does not have parent", jobs[jobs_wo_parents[i]].job_id);
        jobs.erase(jobs.begin() + jobs_wo_parents[i]);
      }
    } while (!jobs_wo_parents.empty());
    return;
  }

  void removeJobs(std::vector<Job> &jobs, int free_cpu_sum) {
    int total_jobs = jobs.size();
    for (int i = total_jobs-1; i >= 0; i--) {
      if (jobs[i].num_cpus > free_cpu_sum) {
        XBT_INFO("Removing job %i", jobs[i].job_id);
        jobs.erase(jobs.begin()+i);
      }
    }
    // remove jobs without parents
    removeJobsWoParents(jobs);
  }

  void operator()()
  {
    // remove the jobs that can not be run
    // jobs that require more resource than available
    int free_cpu_sum = getFreeCpus();
    for (int i=0; i < SlurmDs.size(); i++) {
      free_cpus[i] = 0;
    }
    for (int i=0; i < SlurmDs.size(); i++) {
      SlurmDs[i]->put(new Job(), communicate_cost);
    }
    XBT_INFO("Total number of CPUs %i", free_cpu_sum);
    removeJobs(jobs, free_cpu_sum);

    while (jobs.size() > 0) {
      // while there are pending jobs
      // find the number of free cpus across all the nodes
      free_cpu_sum = getFreeCpus();
      bool job_distributed = false;
      XBT_DEBUG("Number of free cpus in total %i", free_cpu_sum);

      // if there are enough free cpus to execute the job
      if (free_cpu_sum >= jobs[0].num_cpus) {
        // distrbute the job among the nodes
        // currently we are distributing from the start to end
        // round robin or more intelligent criteria can be used
        for (int i=0; i<SlurmDs.size(); i++) {
          if (free_cpus[i] > 0 && !job_distributed) {
            // if there is a free cpu and job is not completely distributed
            int cpus_to_use = std::min(free_cpus[i], jobs[0].num_cpus);
            Job *job_subset = new Job(jobs[0].job_id, cpus_to_use, jobs[0].computation_cost,
                                      jobs[0].priority, jobs[0].p_job_id);
            SlurmDs[i]->put(job_subset, communicate_cost);
            // TODO - all the CPUs wont start the job at the same time
            //        more complex protocol is need to first share all
            //        job information to all the nodes first and then
            //        start the execution
            XBT_INFO("Sent job %i to node %s", jobs[0].job_id, host_name[i].c_str());
            if (cpus_to_use < jobs[0].num_cpus) {
              jobs[0].num_cpus = jobs[0].num_cpus - cpus_to_use;
            } else {
              jobs.erase(jobs.begin());
              job_distributed = true;
            }
          } else {
            // send empty job
            SlurmDs[i]->put(new Job(), communicate_cost);
          }
        }
      } else {
        // execution of this job ain't possible
        // give no work to SlurmDs in this iteration
        for (int i=0; i < SlurmDs.size(); i++) {
          SlurmDs[i]->put(new Job(), communicate_cost);
        }
      }
      for (int i=0; i<SlurmDs.size(); i++) {
        free_cpus[i] = 0;
      }
      sg4::this_actor::sleep_for(1);
    }
    // All jobs have completed
    // Send terminate signal to all the SlurmDs
    XBT_INFO("All jobs completed collecting the final log");
    free_cpu_sum = getFreeCpus();
    XBT_INFO("Number of free cpus in total %i", free_cpu_sum);
    XBT_INFO("Send termination request");
    for (int i=0; i < SlurmDs.size(); i++) {
      SlurmDs[i]->put(new Job(-1,-1,-1,-1), communicate_cost);
    }
    XBT_INFO("SlurmCtlD exiting");
  }
};

class SlurmD {
  long communicate_cost = 100;
  std::vector<sg4::Host*> cpus;
  std::vector<sg4::ExecPtr> executions;
  sg4::Mailbox *mymailbox;
  sg4::Mailbox *masterMailBox;

public:
  explicit SlurmD(std::vector<std::string> args)
  {

    masterMailBox = sg4::Mailbox::by_name(args[1]);
    mymailbox = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());

    for (unsigned int i = 2; i < args.size(); i++) {
      cpus.push_back(sg4::Host::by_name(args[i]));
      executions.push_back(nullptr);
    }

    XBT_INFO("Got %zu cpus", cpus.size());
  }

  void operator()()
  {
    for (;;) {
      /*
      Put the status of cpus to the SlurmCtlD
      Get the computation amount
      if computation amount is received then start those executions on the cpus
      */
      
      // accumulate the cpu info
      int num_free_cpus = 0;
      
      for (int i=0; i < cpus.size(); i++) {
        XBT_VERB("Compuatation load %f on node %d",cpus[i]->get_load(), i);
        if (cpus[i]->get_load() == 0.0) {
          num_free_cpus++;
        }
      }
      mymailbox->put(new int(num_free_cpus), communicate_cost);
      XBT_DEBUG("Number of free CPUs %i", num_free_cpus);

      Job *j = mymailbox->get<Job>();

      if (j->num_cpus > 0) {
        XBT_VERB("Executing job %i on %i cpus of computation %f", j->job_id, j->num_cpus, j->computation_cost);
        for (int i=0; i < cpus.size(); i++) {
          if (cpus[i]->get_load() == 0) {
            double computation = j->computation_cost;
            sg4::ExecPtr exec = sg4::this_actor::exec_init(computation);
            exec->set_host(cpus[i]);
            exec->start();
            executions[i] = exec;
            j->num_cpus--;
            if (j->num_cpus == 0) {
              break;
            }
          }
        }
      } else if (j->num_cpus == 0) {
        // do nothing;
      } else if (j->num_cpus == -1) {
        // end the simulation
        XBT_INFO("Recieved termination request");
        break;
      }
      sg4::this_actor::sleep_for(0.1);
    }

    XBT_INFO("SlurmD EXITING");
  }
};

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  /* Register the classes representing the actors */
  e.register_actor<SlurmCtlD>("slurmctld");
  e.register_actor<SlurmD>("slurmd");

  /* Load the platform description and then deploy the application */
  e.load_platform(argv[1]);
  e.load_deployment(argv[2]);

  /* Run the simulation */
  e.run();

  XBT_INFO("Simulation is over");

  return 0;
}
