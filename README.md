# Simgrid-HPC-simulation
Current working design -
  - SlurmCtlD
    1. Receive the information from all the SlurmD processes.
    2. Find whether the job in front of the Job queue can be run.
    3. If there is enough resource, distribute the compute volume to the SlurmD(s).
  - SlurmD
    1. Send the available free resource to SlurmCtlD (currently only resource considered is the number of free CPUs).
    2. Receive the Job Subset (compute volume and the number of CPUs on which it needs to be run).
    3. Execute asynchronously the compute volume on the free CPUs.

Installation.
  - Ubuntu 22.04 or latest
  - Prerequisites to run the model
    1. simgrid library
      ```
      sudo apt install simgrid pajeng cmake g++
      ```
    2. tensorflow
      1. Install pip
        ```
        sudo apt install python3-pip
        ```
      2. Install tensorflow
        ```
        pip install tensorflow
        ```
    3. Clone the repository
      ```
      git clone https://github.com/gautamMeeshi/Simgrid-HPC-simulation.git
      ```
How to run the model?
  1. Compile the source files - `make compile`
  2. Run the model - `make run SCHED=<scheduler_type> JOB_FILE=<job_file_name>`
    scheduler_type = fcfs_backfill/fcfs/remote_fcfs_bf/remote_nn/remote_qnn/remote_learn_nn

Important links for reference -
  1. [SimGrid host energy consumption plugin](https://simgrid.org/doc/latest/Plugins.html#plugin-host-energy).
  2. This model is inspired by [SimGrid-master-worker](https://simgrid.org/doc/latest/Tutorial_Algorithms.html#discover-the-master-workers) example.