# Simgrid-HPC-simulation
Current working design -
  - SlurmCtlD
    1. Receive the information from all the SlurmD processes. Mark the competed jobs if any.
    2. Send the pending jobs and nodes information to scheduler function.
    3. Scheduler returns node to job mapping.
    3. Master node sends the jobs to be run to the assigned nodes.
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
      2. Install tensorflow (newer version of pip require installation in venv)  
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
    scheduler_type = easy_backfill/fcfs/naive_backfill/remote_nn  
    eg - `make run SCHED=fcfs JOB_FILE=jobs1.csv`  
    job files are located at ./input/jobs  

Important links for reference -
  1. [SimGrid host energy consumption plugin](https://simgrid.org/doc/latest/Plugins.html#plugin-host-energy).
  2. This model is inspired by [SimGrid-master-worker](https://simgrid.org/doc/latest/Tutorial_Algorithms.html#discover-the-master-workers) example.

