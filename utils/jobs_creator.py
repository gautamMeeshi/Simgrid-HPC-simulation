import random
import numpy as np

def BetaRandomInt(a, b, rng):
    '''
    Returns a random number according to a Beta distribution in the range
    '''
    return int(rng[0] + np.random.beta(a, b)*(rng[1]-rng[0]))

def GetNumNodes(nodes_range):
    return BetaRandomInt(1.2, 6, nodes_range)

def GetNumTasksPerNode(tpn_range):
    return random.randint(tpn_range[0], tpn_range[1])

def GetNumCpusPerTask(cpt_range):
    return random.randint(cpt_range[0], cpt_range[1])

def GetComputation(runtime_range, nodes, tpn, cpt):
    '''
    computation range is such that the runtime
    is in the range of 15 mins to 6 hours
    '''
    def __runtime2comp(runtime):
        return runtime*nodes*(tpn*cpt if tpn*cpt < 12 else 12)
    # get runtime
    runtime = BetaRandomInt(1.2, 4, runtime_range)
    # convert runtime to computation volume
    return __runtime2comp(runtime)

def CreateJobFile(file_path):
    runtime_range = (15*60, 12*60*60) # runtime in seconds
    nodes_range = (1, 60)
    tasks_per_node_range = (1,18)
    cpus_per_task_range = (1,4)
    dependency_range = (0,5)
    num_jobs = 400
    injection_time = 0

    file = open(file_path,'w+')
    file.write('job_id,nodes,tasks-per-node,cpu-per-task,total_computation,priority,injection_time,dependency\n')

    for i in range(num_jobs):
        dependency_set = set()
        num_dependencies = BetaRandomInt(1.1, 3.8, dependency_range)
        if (i>0):
            for j in range(num_dependencies):
                dependency_set.add(random.randint(0,i-1))
        dependency_string = ''
        for d in dependency_set:
            dependency_string += str(d)+','
        dependency_string = dependency_string[:-1]
        nodes = GetNumNodes(nodes_range)
        tpn = GetNumTasksPerNode(tasks_per_node_range)
        cpt = GetNumCpusPerTask(cpus_per_task_range)
        computation = str(GetComputation(runtime_range, nodes, tpn, cpt)*2)+'e9' # because frequency of CPU is 2GHz

        file.write(f'''{i},{nodes},{tpn},{cpt},{computation},1,{injection_time},({dependency_string})''')
        if (i<num_jobs-1):
            file.write('\n')
        if ((i+1)%10 == 0):
            injection_time += 3600 # input the jobs every 1 hour

for i in range(0,400):
    CreateJobFile(f'../input/jobs/jobs{i+1}.csv')
