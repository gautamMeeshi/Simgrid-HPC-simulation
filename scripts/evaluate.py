import subprocess
import time

class Stats:
    def __init__(self):
        return
    def __str__(self):
        res = []
        for k,v in self.__dict__.items():
            res.append(f'{k} :  {v}')
        return '\n'.join(res)

def extractStats(stdout):
    res = Stats()
    # find line containing the string 'Total energy consumed'
    stdout = stdout.split('\n')
    temp = list(filter( lambda x: 'Total energy consumed' in x, stdout))
    assert(len(temp) == 1)
    res.energy = float(temp[0].split()[-1][:-2])
    # find line containing the string 'runtime'
    temp = list(filter( lambda x: 'Total execution time' in x, stdout))
    res.runtime = float(temp[0].split()[-1][:-1])
    temp = list(filter( lambda x: 'EDP of the system' in x, stdout))
    res.edp = float(temp[0].split()[-1][:-3])
    temp = list(filter( lambda x: 'Resource utilization' in x, stdout))
    res.resrc_util = float(temp[0].split()[-1][:-1])
    temp = list(filter( lambda x: 'Average waiting time' in x, stdout))
    res.avg_wait_time = float(temp[0].split()[-1][:-1])
    return res

def run100():
    total_jobs_run = 0
    for i in range(5,41):
        try:
            print('-'*10, f'Running jobs{i}.csv','-'*10)
            # print(f'Running fcfs')
            # result = subprocess.run(['make', 'run', 'SCHED=fcfs', f'JOB_FILE=jobs{i}.csv'],
            #                         check=True, capture_output=True, text=True)
            # fcfs_stats = extractStats(result.stderr)
            # print(fcfs_stats)
            # print(f'Running easy_bf')
            # result = subprocess.run(['make', 'run', 'SCHED=easy_backfill', f'JOB_FILE=jobs{i}.csv'],
            #                         check=True, capture_output=True, text=True)
            # fcfs_bf_stats = extractStats(result.stderr)
            # print(fcfs_bf_stats)
            # print('Running naive_bf')
            # result = subprocess.run(['make', 'run', 'SCHED=naive_backfill', f'JOB_FILE=jobs{i}.csv'],
            #                         check=True, capture_output=True, text=True)
            # aggr_bf_stats = extractStats(result.stderr)
            # print(aggr_bf_stats)
            print(f'Running remote_nn')
            attempts = 8
            nn_stats = None
            while attempts>0:
                try:
                    result = subprocess.run(['make', 'run', 'SCHED=remote_nn', f'JOB_FILE=jobs{i}.csv'],
                                            check=True, capture_output=True, text=True)
                    nn_stats = extractStats(result.stderr)
                    attempts = 0
                except:
                    print("Remote nn failed, retrying")
                    attempts -=1
                    time.sleep(8)
            if nn_stats == None:
                continue
            total_jobs_run +=1
            print(nn_stats)
        except subprocess.CalledProcessError as e:
            print(f"Command {e.cmd} failed with return code:", e.returncode)
            print("Error output:", e.stderr)
if __name__ == "__main__":
    run100()
