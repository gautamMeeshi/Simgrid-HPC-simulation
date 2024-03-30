#include <simgrid/s4u.hpp>
#include <simgrid/plugins/energy.h>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include "helper.hpp"
#include "scheduler.hpp"
#include "objects.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(HPC, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

class SlurmCtlD {
  /*
  Collects the information from the SlurmDs
  Distributes the queue of work to SlurmD cores
  */

  std::string job_file_name = "";
  long communicate_cost = 100;
  long jobs_remaining = 0;
  std::vector<sg4::Mailbox*> SlurmDs;
  sg4::Mailbox *mymailbox;
  std::map<int, Job*> jobs;
  std::vector<Resource> resrcs;
  std::vector<std::string> host_name;
  std::string scheduler_type;
public:
  explicit SlurmCtlD(std::vector<std::string> args)
  {
    // xbt_assert(args.size() > 4, "The master function expects 3 arguments plus the workers' names");

    XBT_INFO("SlurmCtlD scheduler type %s", args[2].c_str());
    
    job_file_name = args[1];
    scheduler_type = args[2];
    mymailbox = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());
    jobs = parseJobFile(job_file_name);

    for (unsigned int i = 3; i < args.size(); i++) {
      SlurmDs.push_back(sg4::Mailbox::by_name(args[i]));
      resrcs.push_back(Resource());
      host_name.push_back(args[i]);
    }

    XBT_INFO("Got %zu slurmds", SlurmDs.size());
  }

  int getFreeCpus() {
    int free_cpu_sum = 0;
    for (int i=0; i < SlurmDs.size(); i++) {
      SlurmDmsg* msg = SlurmDs[i]->get<SlurmDmsg>();
      resrcs[i].free_cpus = msg->free_cpus;
      free_cpu_sum += resrcs[i].free_cpus;

      for (int j=0; j<msg->jobs_completed.size(); j++) {
        if (msg->jobs_completed[j] != -1 &&
            jobs[msg->jobs_completed[j]]->job_state != COMPLETED) {
            jobs[msg->jobs_completed[j]]->job_state = COMPLETED;
            XBT_INFO("Completed job %i", msg->jobs_completed[j]);
        }
      }
      delete msg;
    }
    return free_cpu_sum;
  }

  void removeJobsWoParents() {
    std::vector<int> jobs_wo_parents;
    do {
      jobs_wo_parents.clear();
      for (auto it = jobs.begin(); it != jobs.end(); it++) {
        for (int i=0; i < it->second->p_job_id.size(); i++) {
          if (jobs.find(it->second->p_job_id[i]) == jobs.end()) {
            jobs_wo_parents.push_back(it->first);
            break;
          }
        }
      }
      for (int i=0; i<jobs_wo_parents.size(); i++) {
        XBT_INFO("Removing job %i, does not have parent", jobs[jobs_wo_parents[i]]->job_id);
        jobs.erase(jobs_wo_parents[i]);
      }
    } while (!jobs_wo_parents.empty());
    return;
  }

  void removeJobs(int free_cpu_sum) {
    std::vector<int> jobs_to_remove;
    for (auto it = jobs.begin(); it != jobs.end(); it++) {
      if (it->second->num_cpus > free_cpu_sum) {
        jobs_to_remove.push_back(it->first);
      }
    }
    for (int i=0; i<jobs_to_remove.size(); i++) {
      XBT_INFO("Removing job %i, not enough resource", jobs[jobs_to_remove[i]]->job_id);
      jobs.erase(jobs_to_remove[i]);
    }
    // remove jobs without parents
    removeJobsWoParents();
  }

  void operator()()
  {
    // remove the jobs that can not be run
    // jobs that require more resource than available
    int free_cpu_sum = getFreeCpus();
    for (int i=0; i < SlurmDs.size(); i++) {
      resrcs[i].free_cpus = 0;
    }
    for (int i=0; i < SlurmDs.size(); i++) {
      SlurmDs[i]->put(new SlurmCtlDmsg(), communicate_cost);
    }
    XBT_INFO("Total number of CPUs %i", free_cpu_sum);
    removeJobs(free_cpu_sum);
    jobs_remaining = jobs.size(); // ASSUMPTION - all the jobs are in PENDING state

    while (jobs_remaining > 0) {
      // while there are pending jobs
      // find the number of free cpus across all the nodes
      free_cpu_sum = getFreeCpus();
      bool job_distributed = false;
      XBT_DEBUG("Number of free cpus in total %i", free_cpu_sum);

      std::vector<SlurmCtlDmsg*> scheduled_jobs = scheduler(jobs, resrcs, scheduler_type, jobs_remaining);
      xbt_assert(scheduled_jobs.size() == SlurmDs.size(),
                 "Scheduler output size not same as the number of SlurmDs");
      for (int i=0; i<SlurmDs.size(); i++) {
        SlurmDs[i]->put(scheduled_jobs[i], communicate_cost);
      }
      for (int i=0; i<SlurmDs.size(); i++) {
        resrcs[i].free_cpus = 0;
      }
      sg4::this_actor::sleep_for(3);
    }
    // All jobs have completed
    // Send terminate signal to all the SlurmDs
    XBT_INFO("All jobs scheduled collecting the final log");
    free_cpu_sum = getFreeCpus();
    XBT_INFO("Number of free cpus in total %i", free_cpu_sum);
    for (int i=0; i < SlurmDs.size(); i++) {
      SlurmDs[i]->put(new SlurmCtlDmsg(STOP), communicate_cost);
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
  std::vector<int> running_job_ids;

public:
  explicit SlurmD(std::vector<std::string> args)
  {

    masterMailBox = sg4::Mailbox::by_name(args[1]);
    mymailbox = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());

    for (unsigned int i = 2; i < args.size(); i++) {
      cpus.push_back(sg4::Host::by_name(args[i]));
      executions.push_back(nullptr);
      running_job_ids.push_back(-1);
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
      std::vector<int> jobs_completed;
      for (int i=0; i < cpus.size(); i++) {
        XBT_VERB("Computation load %f on node %d",cpus[i]->get_load(), i);
        if (cpus[i]->get_load() == 0.0) {
          if (running_job_ids[i] != -1) {
            jobs_completed.push_back(running_job_ids[i]);
            running_job_ids[i] = -1;
          }
          num_free_cpus++;
        }
      }
      mymailbox->put(new SlurmDmsg(num_free_cpus, jobs_completed), communicate_cost);
      XBT_DEBUG("Number of free CPUs %i", num_free_cpus);

      SlurmCtlDmsg *msg = mymailbox->get<SlurmCtlDmsg>();

      if (msg->sig == RUN) {
        // execute the provided jobs
        xbt_assert(msg->jobs.size() != 0);
        for (int k=0; k < msg->jobs.size(); k++) {
          Job *j = msg->jobs[k];
          XBT_INFO("Executing job %i on %i cpus of computation %f",
                   j->job_id, j->num_cpus, j->computation_cost);
          for (int i=0; i < cpus.size(); i++) {
            if (cpus[i]->get_load() == 0 && running_job_ids[i] == -1) {
              running_job_ids[i] = j->job_id;
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
          delete j;
        }
      } else if (msg->sig == IDLE) {
        // do nothing;
      } else if (msg->sig == STOP) {
        // end the simulation
        break;
      }
      delete msg;
      sg4::this_actor::sleep_for(0.1);
    }

    XBT_INFO("SlurmD EXITING");
  }
};

void generateStats(sg4::Engine &e, double exec_time) {
  std::ofstream stats_file("stats.csv");
  XBT_INFO("---------STATS--------");
  std::vector<sg4::Host*> all_hosts= e.get_all_hosts();
  double total_energy_consumption = 0.0;
  for (int i=0; i<all_hosts.size(); i++) {
    double energy_consumption = sg_host_get_consumed_energy(all_hosts[i]);
    stats_file << all_hosts[i]->get_name() <<',' << energy_consumption<<'\n';
    total_energy_consumption += energy_consumption;
  }
  stats_file << "total_energy_consumption,"<<total_energy_consumption<<"J"<<'\n';
  stats_file << "total_execution_time,"<<exec_time<<"s"<<'\n';
  XBT_INFO("Total energy consumed by CPUs = %lfJ", total_energy_consumption);
  XBT_INFO("Total exection time %lfs", exec_time);
  stats_file.close();
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  /* Register the classes representing the actors */
  e.register_actor<SlurmCtlD>("slurmctld");
  e.register_actor<SlurmD>("slurmd");

  /* Load the platform description and then deploy the application */
  sg_host_energy_plugin_init();

  e.load_platform(argv[1]);
  e.load_deployment(argv[2]);

  /* Run the simulation */
  double start_time = sg4::Engine::get_clock();
  e.run();
  double end_time = sg4::Engine::get_clock();
  generateStats(e, end_time-start_time);
  XBT_INFO("Simulation is over");
  

  return 0;
}
