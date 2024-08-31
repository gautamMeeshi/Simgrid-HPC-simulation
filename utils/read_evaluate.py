file = open('../evaluate.txt', 'r')

lines = file.readlines()

jobs = []
job = []
for line in lines:
    if 'Running jobs' in line:
        if len(job) > 0:
            jobs.append(job)
            job = []
    else:
        job.append(line)

if len(job) > 0:
    jobs.append(job)

def extract_metrics(job):
    fcfs = []
    aggr = []
    nn = []
    sched_type = ''
    for line in job:
        if 'fcfs' in line:
            sched_type = 'fcfs'
            continue
        elif 'aggr' in line:
            sched_type = 'aggr'
            continue
        elif 'nn3' in line:
            sched_type = 'nn'
            continue
        (metric, _, value) = line.split()
        if sched_type == 'fcfs':
            fcfs.append((metric, float(value)))
        elif sched_type == 'aggr':
            aggr.append((metric, float(value)))
        elif sched_type == 'nn':
            nn.append((metric, float(value)))
    ranks = {}
    for i in range(len(fcfs)):
        vals = [fcfs[i], aggr[i], nn[i]]
        vals.sort()
        if fcfs[i][0] == 'resrc_util':
            vals.reverse()
        ranks[fcfs[i][0]] = (vals.index(fcfs[i]), vals.index(aggr[i]), vals.index(nn[i]))
    return ranks

all_ranks = []
for job in jobs:
    all_ranks.append(extract_metrics(job))
keys = list(all_ranks[0].keys())
output = open('ranks.csv', 'w')
output.write('job,')
winner = []
avg = ([],[],[])
for i in range(len(keys)):
    output.write(keys[i])
    if i < len(keys)-1:
        output.write(',')
    winner.append(0)
    for j in range(3):
        avg[j].append(0)

output.write('\n')
job_num = 1
for rank in all_ranks:
    output.write(str(job_num)+',')
    job_num+=1
    for i in range(len(keys)):
        output.write(str(rank[keys[i]]))
        if rank[keys[i]][2] == 0:
            winner[i] += 1
        for k in range(3):
            avg[k][i] += rank[keys[i]][k]
        if i < len(keys)-1:
            output.write(',')
        else:
            output.write('\n')

output.write('wins,')
for i in range(len(keys)):
    output.write(str(winner[i]))
    if i < len(keys)-1:
        output.write(',')
output.write('\n')
output.write('avg_fcfs,')
for i in range(len(keys)):
    output.write(str(avg[0][i]/len(jobs)))
    if i < len(keys)-1:
        output.write(',')
output.write('\n')

output.write('avg_aggr,')
for i in range(len(keys)):
    output.write(str(avg[1][i]/len(jobs)))
    if i < len(keys)-1:
        output.write(',')
output.write('\n')

output.write('avg_nn,')
for i in range(len(keys)):
    output.write(str(avg[2][i]/len(jobs)))
    if i < len(keys)-1:
        output.write(',')
output.write('\n')