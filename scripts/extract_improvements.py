import subprocess
import os
import time

class Stats:
    def __init__(self):
        return
    def __str__(self):
        res = []
        for k,v in self.__dict__.items():
            res.append(f'{k} :  {v}')
        return '\n'.join(res)

def killPython3Processes():
    ret = subprocess.run(['lsof', '-i'], check=True, capture_output=True, text=True)
    stdout = ret.stdout.split('\n')
    stdout = list(map(lambda x: x.split(), ret.stdout.split('\n')))
    pid = -1
    for row in stdout:
        if (len(row) > 2):
            if row[0] == 'python3':
                pid = row[1]
    if pid != -1:
        print(f'killing {pid}')
        ret = subprocess.run(['kill','-9', pid], check=True, capture_output=True, text=True)
        return (ret.returncode == 0)
    else:
        return True

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
    return res

def runRemoteScheduler(sched, job_file_idx, previous_edp = None):
        attempts = 8
        stats = None
        print(f'Running {sched}')
        while attempts > 0:
            killPython3Processes()
            time.sleep(8)
            try:
                result = subprocess.run(['make', 'run', f'SCHED={sched}', f'JOB_FILE=jobs{job_file_idx}.csv'],
                                        check=True, capture_output=True, text=True)
                stats = extractStats(result.stderr)
                print(stats)
                if previous_edp == None or previous_edp > stats.edp:
                    subprocess.run(['mv', './output/run_log.csv', f'./improvements/run_log{job_file_idx}.csv'])
                attempts = 0
            except Exception as err:
                print(f"retrying {sched}")
                attempts -= 1
        return stats

def run100():
    for i in range(1, 501):
        print('-'*10, f'Running jobs{i}.csv','-'*10)
        stats = runRemoteScheduler('remote_fcfs', i)
        edp = None if stats is None else stats.edp
        stats = runRemoteScheduler('remote_fcfs_bf', i, edp)
        edp = None if stats is None else stats.edp
        stats = runRemoteScheduler('remote_aggressive_bf', i, edp)
        edp = None if stats is None else stats.edp
        runRemoteScheduler('remote_nn3', i, edp)


if __name__ == "__main__":
    run100()
