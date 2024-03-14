#include <simgrid/s4u.hpp>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>
#include "helper.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

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

public:
  explicit SlurmCtlD(std::vector<std::string> args)
  {
    // xbt_assert(args.size() > 4, "The master function expects 3 arguments plus the workers' names");
    XBT_INFO("SlurmCltD args size %zu", args.size());
    job_file_name = args[1];
    mymailbox = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());
    jobs = ParseJobFile(job_file_name);

    for (unsigned int i = 2; i < args.size(); i++) {
      SlurmDs.push_back(sg4::Mailbox::by_name(args[i]));
      free_cpus.push_back(0);
    }

    XBT_INFO("Got %zu slurmds", SlurmDs.size());
  }

  void operator()()
  {
    /*
    While there are pending jobs
      1) Get the pending job
      2) Find the jobs to resource mapping
          Ultimately send the job subset to the SlurmD
          Job subset is a job object with num_cpus same as the number of CPUs on the SlurmD
          
      3) Launch the jobs on those resources
    Once there are no pending jobs info the SlurmD to switch off all their cores

    We need to create a protocol to communicate the job info

    Periodically send and receive messages
    */
    while (jobs.size() > 0) {
      // job front = jobs[0];
      // find the total available resources
      int free_cpu_sum = 0;
      for (int i=0; i<SlurmDs.size(); i++) {
        auto msg = SlurmDs[i]->get<int>();
        free_cpus[i] = *msg;
        free_cpu_sum += free_cpus[i];
      }
      bool job_distributed = false;
      XBT_INFO("Number of free cpus in total %i", free_cpu_sum);
      if (free_cpu_sum > jobs[0].num_cpus) {
        for (int i=0; i<SlurmDs.size(); i++) {
          if (free_cpus[i] > 0 && !job_distributed) {
            int cpus_to_use = std::min(free_cpus[i], jobs[0].num_cpus);
            Job *job_subset = new Job(jobs[0].job_id, cpus_to_use, jobs[0].computation_cost,
                                      jobs[0].priority, jobs[0].p_job_id);
            SlurmDs[i]->put(job_subset, communicate_cost);
            XBT_INFO("Sent job %i to node %i", jobs[0].job_id, i);
            if (cpus_to_use < jobs[0].num_cpus) {
              jobs[0].num_cpus = jobs[0].num_cpus - cpus_to_use;
            } else {
              jobs.erase(jobs.begin());
              job_distributed = true;
            }
          } else {
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
    }
    // All jobs have completed
    // Send terminate signal to all the SlurmDs
    for (int i=0; i < SlurmDs.size(); i++) {
      auto msg = SlurmDs[i]->get_unique<int>();
    }
    for (int i=0; i < SlurmDs.size(); i++) {
      SlurmDs[i]->put(new Job(-1,0,-1,-1), communicate_cost);
    }
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
        if (cpus[i]->get_load() == 0) {
          num_free_cpus++;
        }
      }
      XBT_INFO("Number of free CPUs %i", num_free_cpus);
      mymailbox->put(new int(num_free_cpus), communicate_cost);

      Job *j = mymailbox->get<Job>();

      if (j->num_cpus > 0) {
        XBT_INFO("Executing job %i on %i cpus", j->job_id, j->num_cpus);
        for (int i=0; i < cpus.size(); i++) {
          if (cpus[i]->get_load() == 0) {
            sg4::ExecPtr exec = sg4::this_actor::exec_init(j->computation_cost);
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
        break;
      }
    }
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
