import numpy as np

NUM_NODES = 150
MAX_COMPUTATION = (700*10**12)
CHILD_IMP = 0.2

def UpdateJob2NNDict(JOB_DICT, CUM_RUNTIME, this_job, val, MAX_RUNTIME):
    '''
    recursive function to update the dependent jobs field of JOB_2_NN_DICT
    '''
    parent_jobs = JOB_DICT[this_job][8]
    CUM_RUNTIME[this_job] += val
    val = JOB_DICT[this_job][6]/MAX_RUNTIME*val
    for p in parent_jobs:
        UpdateJob2NNDict(JOB_DICT, CUM_RUNTIME, p, val, MAX_RUNTIME)
    return

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
