import subprocess
import time
import os
import sys
import random
import resource

TREE_TYPES = ["random", "deep", "caterpillar", "caterpillar_with_trees", "misc_1", "misc_2"]
ALGORITHMS = ["euler-tour", "micro-trees"]
BASE_ALGO = "small-to-large"
BRUTE_ALGO = "brute-force"

BUILD_DIR = "../build"
GEN = os.path.join(BUILD_DIR, "generator")
SOLVER = os.path.join(BUILD_DIR, "decremental_tree_connectivity")

GREEN = "\033[92m"
RED = "\033[91m"
BLUE = "\033[94m"
CYAN = "\033[96m"
BOLD = "\033[1m"
RESET = "\033[0m"

def set_unlimited_stack():
    try:
        resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
    except Exception:
        pass

def run_command(cmd, stdin_file):
    time_cmd = ["/usr/bin/time", "-f", "%M"]
    try:
        proc = subprocess.run(
            time_cmd + cmd,
            stdin=open(stdin_file, 'r'),
            capture_output=True,
            text=True,
            check=True
        )
        mem_kb = int(proc.stderr.strip().split('\n')[-1])
        return proc.stdout, mem_kb, 0
    except Exception:
        return "", 0, 1

def get_best_stats(algo, stdin_file):
    best_time = float('inf')
    best_mem = 0
    best_out = ""
    run_command([SOLVER, algo], stdin_file)
    for _ in range(3):
        start = time.perf_counter()
        out, mem, rc = run_command([SOLVER, algo], stdin_file)
        duration = (time.perf_counter() - start) * 1000
        if rc != 0: return "", 0, 0, 1
        if duration < best_time:
            best_time = duration
            best_mem = mem
            best_out = out
    return best_out, best_time, best_mem, 0

def print_header(mode, n, m, iteration, total_iters):
    print(f"\n{BOLD}{BLUE}=== DECREMENTAL CONNECTIVITY {mode.upper()} ==={RESET}")
    print(f"{CYAN}Iteration: {iteration}/{total_iters} | Nodes: {n} | Queries: {m}{RESET}\n")
    header = f"{'ALGORITHM':<20} | {'STATUS':<10} | {'TIME (ms)':<10} | {'MEMORY (KB)':<12}"
    print(BOLD + header + RESET)
    print("-" * len(header))

def benchmark(mode, iterations):
    set_unlimited_stack()
    if mode == "correctness_test":
        n, m, ref_algo = 1000, 5000, BRUTE_ALGO
        test_algos = ALGORITHMS + [BASE_ALGO]
    else:
        n, m, ref_algo = 1000000, 2000000, BASE_ALGO
        test_algos = ALGORITHMS

    for i in range(1, iterations + 1):
        print_header(mode, n, m, i, iterations)
        for tree_type in TREE_TYPES:
            seed = random.randint(0, 1000000)
            test_in = os.path.join(BUILD_DIR, "test.in")
            with open(test_in, "w") as f:
                subprocess.run([GEN, str(n), str(m), str(seed), tree_type], stdout=f, check=True)
            
            print(f"{BOLD}Tree: {tree_type.upper()} (Seed: {seed}){RESET}")
            
            ref_out, ref_time, ref_mem, rc_ref = get_best_stats(ref_algo, test_in)
            if rc_ref != 0:
                print(f"{RED}{ref_algo:<20} | FAILED (STOPS){RESET}")
                sys.exit(1)
            
            print(f"{CYAN}{ref_algo:<20}{RESET} | {CYAN}BASELINE{RESET}   | {ref_time:>9.2f} | {ref_mem:>11}")

            current_algos = list(test_algos)
            random.shuffle(current_algos)

            for algo in current_algos:
                out, duration, mem, rc = get_best_stats(algo, test_in)
                
                if rc != 0:
                    print(f"{BOLD}{algo:<20}{RESET} | {RED}CRASHED (STOPS){RESET} | {0.00:>9.2f} | {0:>11}")
                    sys.exit(1)
                elif out == ref_out:
                    print(f"{BOLD}{algo:<20}{RESET} | {GREEN}PASSED{RESET}     | {duration:>9.2f} | {mem:>11}")
                else:
                    print(f"{BOLD}{algo:<20}{RESET} | {RED}WRONG (STOPS){RESET}   | {duration:>9.2f} | {mem:>11}")
                    with open(os.path.join(BUILD_DIR, "ref.out"), "w") as f: f.write(ref_out)
                    with open(os.path.join(BUILD_DIR, "algo.out"), "w") as f: f.write(out)
                    sys.exit(1)
            print("-" * 60)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python validate.py [stress_test | correctness_test] [iterations]")
    else:
        mode = sys.argv[1]
        iters = int(sys.argv[2]) if len(sys.argv) > 2 else 1
        benchmark(mode, iters)