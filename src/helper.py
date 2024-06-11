import numpy as np

NUM_NODES = 150
MAX_COMPUTATION = (700*10**12)
CHILD_IMP = 0.2

def FindDependentJobs(jobs_list):
    dependent_jobs = {}
    for i in range(0,len(jobs_list)):
        dependent_jobs[jobs_list[i][0]] = []
    for i in range(0,len(jobs_list)):
        for j in range(len(jobs_list[i][6])):
            dependent_jobs[jobs_list[i][6][j]].append(jobs_list[i][0])
    return dependent_jobs

def DFS(root, comp_dict, dependent_jobs, cumulative_comp):
    if root in cumulative_comp:
        return cumulative_comp[root]
    cum_comp = 0
    for child in dependent_jobs[root]:
        cum_comp += DFS(child, comp_dict, dependent_jobs, cumulative_comp)
    cumulative_comp[root] = comp_dict[root] + CHILD_IMP*cum_comp
    return cumulative_comp[root]

def FindCumulativeComp(jobs_list, dependent_jobs):
    # DFS over the job graph
    cumulative_comp = {}
    comp_dict = {}
    for i in range(0,len(jobs_list)):
        comp_dict[jobs_list[i][0]] = jobs_list[i][5]
    for i in range(0,len(jobs_list)):
        if (jobs_list[i][0] not in cumulative_comp):
            DFS(jobs_list[i][0], comp_dict, dependent_jobs, cumulative_comp)
    # Normalize the cumulative comp
    max_cc = max(cumulative_comp.values())
    for k in cumulative_comp:
        cumulative_comp[k] = cumulative_comp[k]/max_cc
    return cumulative_comp

def GetNNInput(free_nodes_str, jobs_list):
    X = []
    for i in free_nodes_str:
        if i == '1':
            X.append(1)
        else:
            X.append(0)
    for i in range(64):
        if i < len(jobs_list):
            X.append(jobs_list[i][2]/NUM_NODES)
            X.append((jobs_list[i][5]*jobs_list[i][2]*jobs_list[i][3]*jobs_list[i][4])/MAX_COMPUTATION)
        else:
            X.append(0)
            X.append(0)
    X = np.array([X])
    return X
