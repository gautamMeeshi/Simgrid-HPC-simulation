   tracing: Enable the tracing system. You have to enable this option to use other tracing options.
       Type: boolean; Current value: 0
   tracing/actor: Trace the behavior of all categorized actors, grouping them by host. Can be used to track actor location if the simulator does actor migration.
       Type: boolean; Current value: 0
   tracing/basic: Avoid extended events (impoverished trace file).
       Type: boolean; Current value: 0
   tracing/categorized: Trace categorized resource utilization of hosts and links.
       Type: boolean; Current value: 0
   tracing/comment: Add a comment line to the top of the trace file.
       Type: string; Current value: 
   tracing/comment-file: Add the contents of a file as comments to the top of the trace.
       Type: string; Current value: 
   tracing/disable-destroy: Disable platform containers destruction.
       Type: boolean; Current value: 0
   tracing/disable_link: Do not trace link bandwidth and latency.
       Type: boolean; Current value: 0
   tracing/disable_power: Do not trace host power.
       Type: boolean; Current value: 0
   tracing/filename: Trace file created by the instrumented SimGrid.
       Type: string; Current value: simgrid.trace
   tracing/platform: Register the platform in the trace as a hierarchy.
       Type: boolean; Current value: 0
   tracing/platform/topology: Register the platform topology in the trace as a graph.
       Type: boolean; Current value: 1
   tracing/precision: Numerical precision used when timestamping events (expressed in number of digits after decimal point)
       Type: int; Current value: 6
   tracing/smpi: Tracing of the SMPI interface.
       Type: boolean; Current value: 0
   tracing/smpi/computing: Generate 'Computing' states to trace the out-of-SMPI parts of the application
       Type: boolean; Current value: 0
   tracing/smpi/display-sizes: Add message size information (in bytes) to the to links and states (SMPI only). For collectives, it usually corresponds to the total number of bytes sent by a process.
       Type: boolean; Current value: 0
   tracing/smpi/format: Select trace output format used by SMPI. The default is the 'Paje' format. The 'TI' (Time-Independent) format allows for trace replay.
       Type: string; Current value: Paje
   tracing/smpi/format/ti-one-file: (smpi only) For replay format only : output to one file only
       Type: boolean; Current value: 0
   tracing/smpi/group: Group MPI processes by host.
       Type: boolean; Current value: 0
   tracing/smpi/internals: Generate tracing events corresponding to point-to-point messages sent by SMPI collective communications
       Type: boolean; Current value: 0
   tracing/smpi/sleeping: Generate 'Sleeping' states for the sleeps in the application that do not pertain to SMPI
       Type: boolean; Current value: 0
   tracing/uncategorized: Trace uncategorized resource utilization of hosts and links. To use if the simulator does not use tracing categories but resource utilization have to be traced.
       Type: boolean; Current value: 0
   tracing/vm: Trace the behavior of all virtual machines.
       Type: boolean; Current value: 0