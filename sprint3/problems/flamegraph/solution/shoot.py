import argparse
import subprocess
import time
import random
import shlex

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1

def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server

def start_perf(pid):
    return f"perf record -g -p {pid} -o perf.data"

def start_flamegraph():
    svg = open("graph.svg", "w")
    perf = subprocess.Popen(("perf", "script", "-i", "perf.data"), stdout=subprocess.PIPE)
    stackcollapse = subprocess.Popen("./FlameGraph/stackcollapse-perf.pl", stdin=perf.stdout, stdout=subprocess.PIPE)
    flamegraph = subprocess.Popen("./FlameGraph/flamegraph.pl", stdin=stackcollapse.stdout, stdout=svg)
    return flamegraph

def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


server = run(start_server())
perf = run(start_perf(server.pid))
make_shots()
stop(perf)
stop(server)
perf.wait()
flamegraph = start_flamegraph()
flamegraph.wait()
time.sleep(1)
print('Job done')
