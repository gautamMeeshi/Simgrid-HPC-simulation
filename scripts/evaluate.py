import subprocess

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
log_file = open('log', 'a')
def run100():
    global log_file
    for i in range(1,20):
        try:
            print('-'*10, f'Running jobs{i}.csv','-'*10)
            print(f'Running fcfs_bf')
            log_file.write(f'job{i}')
            result = subprocess.run(['make', 'run', 'SCHED=fcfs_backfill', f'JOB_FILE=jobs{i}.csv'],
                                    check=True, capture_output=True, text=True)
            fcfs_bf_stats = extractStats(result.stderr)
            print(fcfs_bf_stats)
            log_file.write(f'{fcfs_bf_stats.energy},{fcfs_bf_stats.runtime},{fcfs_bf_stats.edp},')
            print(f'Running remote_nn')
            result = subprocess.run(['make', 'run', 'SCHED=remote_neural_network', f'JOB_FILE=jobs{i}.csv'],
                                    check=True, capture_output=True, text=True)
            nn_stats = extractStats(result.stderr)
            print(nn_stats)
            log_file.write(f'{nn_stats.energy},{nn_stats.runtime},{nn_stats.edp},')
            energy_improvement = (fcfs_bf_stats.energy-nn_stats.energy)/fcfs_bf_stats.energy*100
            runtime_improvement = (fcfs_bf_stats.runtime-nn_stats.runtime)/fcfs_bf_stats.runtime*100
            edp_improvement = (fcfs_bf_stats.edp-nn_stats.edp)/fcfs_bf_stats.edp*100
            log_file.write(f'''{energy_improvement},{runtime_improvement},{edp_improvement}\n''')
        except subprocess.CalledProcessError as e:
            print(f"Command {e.cmd} failed with return code:", e.returncode)
            print("Error output:", e.stderr)

if __name__ == "__main__":
    run100()
