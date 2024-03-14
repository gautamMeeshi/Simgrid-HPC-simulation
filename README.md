# Simgrid-HPC-simulation
Current working design -
  - SlurmCtlD
    1. Receive the information from all the SlurmD processes.
    2. Find whether the job in front of the Job queue can be run.
    3. If there is enough resource, distribute the compute volume to the SlurmD(s).
  - SlurmD
    1. Send the available free resource to SlurmCltD (currently only resource considered is the number of free CPUs).
    2. Receive the Job Subset (compute volume and the number of CPUs on which it needs to be run).
    3. Execute asynchronously the compute volume on the free CPUs.

TODO -
  1. Scheduler support to SlurmCtlD process will be provided once basic model is working.
  2. Power model and configuration tuning has to be Done.
  3. The topology of interconnection system will be modeled.