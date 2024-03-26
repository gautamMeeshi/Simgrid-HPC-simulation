import random

computation_range = (10, 1000000)
cpu_range = (1, 40)
dependency_ranges = (0,5)
num_jobs = 100

file = open('jobs_created.csv','w+')
file.write('job_id,cpus,computation_volume_per_cpu,priority,dependency\n')

for i in range(num_jobs):
    dependency_set = set()
    num_dependencies = random.randint(dependency_ranges[0],dependency_ranges[1])
    if (i>0):
        for j in range(num_dependencies):
            dependency_set.add(random.randint(0,i-1))
    dependency_string = ''
    for k in dependency_set:
        dependency_string += str(k)+','
    dependency_string = dependency_string[:-1]
    num_cpus = random.randint(cpu_range[0],cpu_range[1])
    computation = random.randint(computation_range[0],computation_range[1])
    file.write(f'''{i},{num_cpus},{computation}e9,1,({dependency_string})''')
    if (i<num_jobs-2):
        file.write('\n')