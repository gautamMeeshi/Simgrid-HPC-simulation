#include <simgrid/s4u.hpp>
#include <simgrid/plugins/energy.h>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include "helper.hpp"
#include "scheduler.hpp"
#include "objects.hpp"
#include <regex>

long TRAINING_INTERVAL = 50000;

XBT_LOG_NEW_DEFAULT_CATEGORY(HPC, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

int started = 0;
int completed = 0;

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
  std::vector<int> node_2_job;
  std::vector<std::string> host_name;
  std::string scheduler_type;
  int num_nodes;
  long runnable_jobs;
  long completed_jobs;
  std::map<int, JobLogs> job_logs;
  Scheduler *scheduler;
  std::vector<sg4::Host*> nodes;
  std::string my_name;
  int counter = 0;
  std::vector<std::vector<std::pair<double, double>>> energy_logs;

public:
  explicit SlurmCtlD(std::vector<std::string> args)
  {
    // xbt_assert(args.size() > 4, "The master function expects 3 arguments plus the workers' names");

    XBT_INFO("SlurmCtlD scheduler type %s", args[2].c_str());

    job_file_name = args[1];
    scheduler_type = args[2];
    my_name = sg4::this_actor::get_host()->get_name();
    mymailbox = sg4::Mailbox::by_name(my_name);
    jobs = parseJobFile(job_file_name);
    scheduler = new Scheduler(scheduler_type, jobs);

    for (unsigned int i = 3; i < args.size(); i++) {
      SlurmDs.push_back(sg4::Mailbox::by_name(args[i]));
      nodes.push_back(sg4::Host::by_name(args[i]));
      resrcs.push_back(Resource());
      host_name.push_back(args[i]);
      node_2_job.push_back(-1);
      energy_logs.push_back(std::vector<std::pair<double, double>>());
    }
    num_nodes = SlurmDs.size();
    XBT_INFO("Total number of nodes %zu", SlurmDs.size());
  }

  int receiveSlurmdMsgs() {
    int free_cpu_sum = 0;
    for (int i=0; i < SlurmDs.size(); i++) {
      if (!nodes[i]->is_on()) {
        resrcs[i].free_cpus = CPUS_PER_NODE;
        resrcs[i].node_state = FREE;
        free_cpu_sum += resrcs[i].free_cpus;
        continue;
      }
      SlurmdMsg* msg = SlurmDs[i]->get<SlurmdMsg>();
      resrcs[i].free_cpus = msg->free_cpus;
      resrcs[i].node_state = msg->state;
      free_cpu_sum += resrcs[i].free_cpus;

      for (int j=0; j<msg->jobs_completed.size(); j++) {
        if (msg->jobs_completed[j] != -1) {
          int c_job_id = msg->jobs_completed[j];
          xbt_assert(jobs[c_job_id]->job_state != COMPLETED);
          jobs[c_job_id]->nodes--;
          if (jobs[c_job_id]->nodes == 0) {
            jobs[c_job_id]->job_state = COMPLETED;
            for (int k=0; k < job_logs[c_job_id].nodes_running.size(); k++) {
              node_2_job[job_logs[c_job_id].nodes_running[k]] = -1;
            }
            XBT_INFO("Completed job %i", c_job_id);
            completed_jobs++;
            job_logs[c_job_id].end_time = sg4::Engine::get_clock();
          }
        }
      }
      delete msg;
    }
    for (int i=0; i < SlurmDs.size(); i++) {
      if (node_2_job[i] != -1) {
        // Although this node got FREE other nodes are running the job with the same job id
        resrcs[i].node_state = BUSY;
      }
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
        delete jobs[jobs_wo_parents[i]];
        jobs.erase(jobs_wo_parents[i]);
      }
    } while (!jobs_wo_parents.empty());
    return;
  }

  void removeJobs() {
    std::vector<int> jobs_to_remove;
    for (auto it = jobs.begin(); it != jobs.end(); it++) {
      if (it->second->nodes > num_nodes) {
        jobs_to_remove.push_back(it->first);
      }
    }
    for (int i=0; i<jobs_to_remove.size(); i++) {
      XBT_INFO("Removing job %i, not enough resource", jobs[jobs_to_remove[i]]->job_id);
      delete jobs[jobs_to_remove[i]];
      jobs.erase(jobs_to_remove[i]);
    }
    // remove jobs without parents
    removeJobsWoParents();
  }

  void preSetup() {
    runnable_jobs = jobs.size();
    completed_jobs = 0;
    for (int i=0; i < jobs.size(); i++) {
      job_logs[jobs[i]->job_id] = JobLogs();
    }
  }

  void recordEnergyStats() {
    if (counter == 1000) {
      counter = 0;
      double node_energy = 0;

      for (int i=0; i < nodes.size(); i++) {
        std::string cpu1_name = regex_replace(host_name[i], std::regex("c0"), "c1");
        node_energy = sg_host_get_consumed_energy(nodes[i]) + sg_host_get_consumed_energy(sg4::Host::by_name(cpu1_name));
        energy_logs[i].push_back({sg4::Engine::get_clock(), node_energy});
      }
    }
    return;
  }

  void storeStats(void) {
    std::ofstream stats_file("output/energy_stats.csv");
    for (int i=0; i < host_name.size(); i++) {
      stats_file << host_name[i];
      if (i == host_name.size()-1) {
        stats_file << '\n';
      } else {
        stats_file << ", ";
      }
    }
    long long int line_no = 0;
    bool end = false;
    while (!end) {
      for (int i=0; i < energy_logs.size(); i++) {
        if (line_no == energy_logs[i].size()) {
          end = true;
          break;
        }
        stats_file << '(' << energy_logs[i][line_no].first << ',' << energy_logs[i][line_no].second << ")";
        if (i == energy_logs.size()-1) {
          stats_file << '\n';
        } else {
          stats_file << ", ";
        }
      }
      line_no++;
    }
    stats_file.close();
    std::ofstream job_stats_file("output/job_stats.csv");
    job_stats_file << "job_id, start_time, end_time, runtime, nodes_run_on\n";
    for (auto it = job_logs.begin(); it != job_logs.end(); it++) {
      job_stats_file << it->first << ", " << it->second.start_time << ", " << it->second.end_time << ", ";
      job_stats_file << it->second.end_time - it->second.start_time <<  ", (";
      for (int i=0; i < it->second.nodes_running.size(); i++) {
        job_stats_file << it->second.nodes_running[i];
        if (i < it->second.nodes_running.size()-1) {
          job_stats_file << ", ";
        }
      }
      job_stats_file << ")\n";
    }
    job_stats_file.close();
  }

  void operator()()
  {
    counter++;
    // remove the jobs that can not be run
    // jobs that require more resource than available
    int free_cpu_sum = receiveSlurmdMsgs();
    bool train = false;
    double curr_time;
    for (int i=0; i < SlurmDs.size(); i++) {
      resrcs[i].free_cpus = 0;
    }
    for (int i=0; i < SlurmDs.size(); i++) {
      SlurmDs[i]->put(new SlurmCtldMsg(), communicate_cost);
    }
    XBT_INFO("Total number of CPUs %i", free_cpu_sum);
    removeJobs();
    preSetup();
    jobs_remaining = jobs.size(); // ASSUMPTION - all the jobs are in PENDING state

    while (jobs_remaining > 0) {
      // while there are pending jobs
      // find the number of free cpus across all the nodes
      curr_time = sg4::Engine::get_clock();
      free_cpu_sum = receiveSlurmdMsgs();
      XBT_DEBUG("Number of free cpus in total %i", free_cpu_sum);
      std::vector<SlurmCtldMsg*> scheduled_jobs = scheduler->schedule(jobs, resrcs, jobs_remaining, curr_time);
      xbt_assert(scheduled_jobs.size() == SlurmDs.size(),
                 "Scheduler output size not same as the number of SlurmDs");
      std::set<int> jobs_scheduled;
      for (int i=0; i<SlurmDs.size(); i++) {
        if (scheduled_jobs[i]->sig != RUN && !nodes[i]->is_on()) {
          continue;
        }
        if (scheduled_jobs[i]->sig == IDLE && node_2_job[i] == -1) {
          if (nodes[i]->is_on() && nodes[i]->get_name() != my_name) {
            scheduled_jobs[i]->sig = SLEEP;
            SlurmDs[i]->put(scheduled_jobs[i], communicate_cost);
            continue;
          }
        } else if (scheduled_jobs[i]->sig == RUN && nodes[i]->is_on() == false) {
          nodes[i]->turn_on();
          SlurmDs[i]->get<SlurmdMsg>(); // to prevent deadlock, has no semantic meaning
        }
        SlurmDs[i]->put(scheduled_jobs[i], communicate_cost);
        if (scheduled_jobs[i]->sig == RUN) {
          for (int j=0; j < scheduled_jobs[i]->jobs.size(); j++) {
            jobs_scheduled.insert(scheduled_jobs[i]->jobs[j]->job_id);
            job_logs[scheduled_jobs[i]->jobs[j]->job_id].nodes_running.push_back(i);
            node_2_job[i] = scheduled_jobs[i]->jobs[j]->job_id;
            resrcs[i].relinquish_time = curr_time + scheduled_jobs[i]->jobs[j]->run_time + 10;
          }
        }
      }
      for (auto& job_id: jobs_scheduled) {
        XBT_INFO("Started job %i", job_id);
        job_logs[job_id].start_time = curr_time;
        jobs[job_id]->start_time = curr_time;
      }

      for (int i=0; i<SlurmDs.size(); i++) {
        resrcs[i].free_cpus = 0;
      }
      sg4::this_actor::sleep_for(3);
      recordEnergyStats();
    }
    // All jobs have completed
    // Send terminate signal to all the SlurmDs
    XBT_INFO("All jobs scheduled");
    delete scheduler;
    while (completed_jobs != runnable_jobs) {
      // wait until all jobs have completed
      free_cpu_sum = receiveSlurmdMsgs();
      for (int i=0; i < SlurmDs.size(); i++) {
        if (nodes[i]->is_on()) {
          SlurmDs[i]->put(new SlurmCtldMsg(IDLE), communicate_cost);
        }
      }
      sg4::this_actor::sleep_for(9);
    }
    for (int i=0; i < nodes.size(); i++) {
      nodes[i]->turn_on();
    }
    // send termination request
    receiveSlurmdMsgs();
    for (int i=0; i < SlurmDs.size(); i++) {
      SlurmDs[i]->put(new SlurmCtldMsg(STOP), communicate_cost);
    }
    // clean up the jobs
    for (auto it = jobs.begin(); it != jobs.end(); it++) {
      delete it->second;
    }
    // collect the logs from the slurmds
    storeStats();

    XBT_INFO("SlurmCtlD exiting");

  }
};

class SlurmD {
  long communicate_cost = 100;
  std::vector<sg4::Host*> cpus;
  // std::map<int, std::vector<sg4::ExecPtr>> executions;
  sg4::Mailbox *mymailbox;
  sg4::Mailbox *masterMailBox;
  std::vector<int> running_job_ids;
  int num_cpus;
  SlurmdState state;
  std::string my_name;
  std::string master_name;

public:
  explicit SlurmD(std::vector<std::string> args)
  {
    master_name = args[1];
    masterMailBox = sg4::Mailbox::by_name(master_name);
    my_name = sg4::this_actor::get_host()->get_name();
    mymailbox = sg4::Mailbox::by_name(my_name);

    for (unsigned int i = 2; i < args.size(); i++) {
      cpus.push_back(sg4::Host::by_name(args[i]));
      running_job_ids.push_back(-1);
    }
    num_cpus = cpus.size();
    XBT_VERB("Got %zu cpus", cpus.size());
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
      for (int i=0; i < num_cpus; i++) {
        XBT_DEBUG("Computation load %f on node %d",cpus[i]->get_load(), i);
        if (cpus[i]->get_load() == 0.0) {
          if (running_job_ids[i] != -1) {
            if (jobs_completed.size()>0) {
              if (jobs_completed.back() != running_job_ids[i]){
                jobs_completed.push_back(running_job_ids[i]);
              }
            } else {
              jobs_completed.push_back(running_job_ids[i]);
            }
            running_job_ids[i] = -1;
          }
          num_free_cpus++;
        }
      }
      if (num_free_cpus == num_cpus) {
        state = FREE;
      } else {
        jobs_completed.clear();
        state = BUSY;
      }
      mymailbox->put(new SlurmdMsg(state, num_free_cpus, jobs_completed), communicate_cost);
      XBT_DEBUG("Number of free CPUs %i", num_free_cpus);
      SlurmCtldMsg *msg = mymailbox->get<SlurmCtldMsg>();
      if (msg->sig == RUN) {
        // execute the provided jobs
        xbt_assert(msg->jobs.size() == 1, "Number of jobs received is not 1 received %li", msg->jobs.size());
        for (int k=0; k < msg->jobs.size(); k++) {
          Job *j = msg->jobs[k];
          int total_threads = j->tasks_per_node * j->cpus_per_task;
          XBT_DEBUG("Executing job %i of threads %i cpus of computation %f",
                   j->job_id, total_threads, j->computation_cost);
          int cpu_idx = 0;
          while(total_threads > 0) {
            if (running_job_ids[cpu_idx] == -1 || running_job_ids[cpu_idx] == j->job_id) {
              running_job_ids[cpu_idx] = j->job_id;
              double computation = j->computation_cost;
              sg4::ExecPtr exec = sg4::this_actor::exec_init(computation);
              exec->set_host(cpus[cpu_idx]);
              cpus[cpu_idx]->turn_on();
              exec->start();
            } else {
              xbt_assert(false, "CPU still executing older a job");
            }
            cpu_idx = (cpu_idx + 1) % num_cpus;
            total_threads--;
          }
          delete j;
        }
      } else if (msg->sig == IDLE) {
        // do nothing;
      } else if (msg->sig == SLEEP) {
        // turn off the cpus
        XBT_DEBUG("Turning off");
        delete msg;
        for (int i=cpus.size()-1; i>=0; i--) {
          cpus[i]->turn_off();
        }
      } else if (msg->sig == STOP) {
        // end the simulation
        delete msg;
        break;
      }
      delete msg;
      sg4::this_actor::sleep_for(0.1);
    }

    XBT_VERB("SlurmD EXITING");
  }
};

void generateStats(sg4::Engine &e, double exec_time) {
  std::ofstream stats_file("output/energy_summary_stats.csv");
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
  XBT_INFO("Total energy consumed by CPUs = %lfMJ", total_energy_consumption/1000000);
  XBT_INFO("Total execution time = %lfs", exec_time);
  XBT_INFO("Average power consumption of the system = %lfKW", total_energy_consumption/exec_time/1000);
  XBT_INFO("Average power consumption per CPU = %lfW", total_energy_consumption/exec_time/300);
  XBT_INFO("EDP of the system = %lfGJs", total_energy_consumption*exec_time/1000000000);
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
  xbt_log_control_set("host_energy.threshold:warning");

  return 0;
}
