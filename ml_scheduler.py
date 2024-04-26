import socket
import json

def fifo_scheduler(json_data):
    '''
    Input - json data
        {
        'free_nodes': bit_string,
        'jobs': [ [j, nn, tpn, cpt, [job_id1, job_id2,..]], ...]
        }
    Output - bit string of length number of jobs 1 denoting run
    '''
    # print(json_data)
    num_free_nodes = 0
    for i in json_data['free_nodes']:
        if i=='1':
            num_free_nodes+=1
    jobs = json_data['jobs']
    output = ''
    for i in range(len(jobs)):
        if (int(jobs[i][1]) < num_free_nodes):
            output += '1'
            num_free_nodes -= int(jobs[i][1])
        else:
            output += '0'
    return output

def create_input(json_data):
    input = ''
    input += json_data['free_nodes']
    
def calculate_job_priority_scores(jobs):
    '''
    returns a dictionary
    key job_id
    value score
    '''
    least_jid = min(jobs.keys())
    Pj = {}
    children = {}
    for jid in jobs:
        Ps = 1/(jid + 1 - least_jid)
        Pc = 1/jobs[jid][4]**0.5
        Pn = 1/jobs[jid][2]
        Pj[jid] = 0.5*Ps + 0.4*Pc + 0.1*Pn
        children[jid] = []
        for pid in jobs[jid][5]:
            children[pid].append(jid)
    Pf = {}
    for jid in jobs:
        Pf[jid] = Pj
        for cid in children:
            Pf[jid] += 0.1/(cid-least_jid)*Pj[cid]
    return Pf

def get_runnable_jobs(jobs):
    '''
    returns a boolean list
    denoting the jobs whose parents have completed
    '''
    res = []
    for jid in jobs:
        plist = jobs[jid][5]
        p_completed = True
        for p in plist:
            if (jobs[p][1] in jobs):
                p_completed = False
                break
        res.append(p_completed)
    return res

def heuristic_scheduler(json_data):
    '''
    Input - json data
        {
        'free_nodes': bit_string,
        'jobs': [ [j, job_state, nn, tpn, cpt, [job_id1, job_id2,..]], ...]
        }
    Output - string containing the command seperated list of job_ids to run
    '''
    num_free_nodes = json_data['free_nodes'].count('1')
    jobs = json_data['jobs']
    jobs = dict([(j[0], j) for j in jobs])
    runnable_jobs = get_runnable_jobs(jobs)
    job_priority_scores = calculate_job_priority_scores(jobs)
    runnable_jobs.sort(key = lambda x: job_priority_scores[x[0]])
    jobs_to_run = ''
    for i in range(len(runnable_jobs)):
        if (int(runnable_jobs[i][2]) < num_free_nodes):
            jobs_to_run += str(jobs[i][0]) + ','
            num_free_nodes -= int(jobs[i][2])
        else:
            break
    return jobs_to_run

PORT = 8080
ADDR = "127.0.0.1"
skt = None

try:
    skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as e:
    print(e)
    print('socket creation failed')
skt.bind((ADDR, PORT))
skt.listen(5)
print("socket is listening")

clientSocket, addr = skt.accept()



while True:
    req = clientSocket.recv(2**10*10).decode()
    res = heuristic_scheduler(json.loads(req))
    clientSocket.send(res.encode())

clientSocket.close()
