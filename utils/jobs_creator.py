import random

computation_range = (500, 700)
nodes_range = (1, 50)
tasks_per_node_range = (1,18)
cpus_per_task_range = (1,4)
dependency_ranges = (0,3)
num_jobs = 100

file = open('jobs_created.csv','w+')
file.write('job_id,nodes,tasks-per-node,cpu-per-task,total_computation,priority,dependency\n')

for i in range(num_jobs):
    dependency_set = set()
    num_dependencies = random.randint(dependency_ranges[0], dependency_ranges[1])
    if (i>0):
        for j in range(num_dependencies):
            dependency_set.add(random.randint(0,i-1))
    dependency_string = ''
    for d in dependency_set:
        dependency_string += str(d)+','
    dependency_string = dependency_string[:-1]
    nodes = random.randint(nodes_range[0], nodes_range[1])
    tpn = random.randint(tasks_per_node_range[0], tasks_per_node_range[1])
    cpt = random.randint(cpus_per_task_range[0], cpus_per_task_range[1])
    computation = random.randint(computation_range[0],computation_range[1])*nodes*tpn*cpt

    file.write(f'''{i},{nodes},{tpn},{cpt},{computation}e7,1,({dependency_string})''')
    if (i<num_jobs-1):
        file.write('\n')