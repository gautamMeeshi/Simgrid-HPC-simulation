import subprocess
import os
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
    res.energy = float(temp[0].split()[-1][:-1])
    # find line containing the string 'runtime'
    temp = list(filter( lambda x: 'Total execution time' in x, stdout))
    res.runtime = float(temp[0].split()[-1][:-1])
    res.edp = res.runtime*res.energy
    return res

def run100():
    for i in range(11,21):
        try:
            print('-'*10, f'Running jobs{i}.csv','-'*10)
            print(f'Running fcfs_bf')
            result = subprocess.run(['make', 'run', 'SCHED=remote_fcfs_bf', f'JOB_FILE=jobs{i}.csv'],
                                    check=True, capture_output=True, text=True)
            fcfs_bf_stats = extractStats(result.stderr)
            print(fcfs_bf_stats)
            # print(f'Running remote_qnn')
            # result = subprocess.run(['make', 'run', 'SCHED=remote_qnn', f'JOB_FILE=jobs{i}.csv'],
            #                         check=True, capture_output=True, text=True)
            # rqnn_stats = extractStats(result.stderr)
            # print(rqnn_stats)
            # if (fcfs_bf_stats.edp > rqnn_stats.edp):
            print(f'Moving the run_log.csv to ./improvements/run_log{i}.csv')
            subprocess.run(['mv', './output/run_log.csv', f'./improvements/run_log{i}.csv'])
            # else:
            #     print(f'Moving the run_log_fcfs_bf.csv to ./improvements/run_log{i}.csv')
            #     subprocess.run(['mv', 'output/run_log_fcfs_bf.csv', f'./improvements/run_log{i}.csv'])
        except subprocess.CalledProcessError as e:
            print(f"Command {e.cmd} failed with return code:", e.returncode)
            print("Error output:", e.stderr)

if __name__ == "__main__":
    run100()
