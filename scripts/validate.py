import subprocess
import time
import os
import random

ALGORITHMS = ["small-to-large", "euler-tour", "micro-trees"]
BASE_ALGO = "small-to-large"
N, M = 1000000, 2000000
ITERATIONS = 1000

GREEN = "\033[92m"
RED = "\033[91m"
BLUE = "\033[94m"
CYAN = "\033[96m"
BOLD = "\033[1m"
RESET = "\033[0m"

def run_command(cmd, stdin_file=None):
    time_cmd = ["/usr/bin/time", "-f", "%M"] 
    full_cmd = time_cmd + cmd
    
    start_time = time.perf_counter()
    try:
        proc = subprocess.run(
            full_cmd,
            stdin=open(stdin_file, 'r') if stdin_file else None,
            capture_output=True,
            text=True,
            check=True
        )
        duration = (time.perf_counter() - start_time) * 1000
        mem_kb = int(proc.stderr.strip().split('\n')[-1])
        return proc.stdout, duration, mem_kb, 0
    except subprocess.CalledProcessError as e:
        return "", 0, 0, e.returncode

def print_header():
    print(f"\n{BOLD}{BLUE}=== DECREMENTAL CONNECTIVITY BENCHMARK ==={RESET}")
    print(f"{CYAN}Nodes: {N} | Queries: {M} | Iterations: {ITERATIONS}{RESET}\n")
    header = f"{'ALGORITHM':<20} | {'STATUS':<10} | {'TIME (ms)':<10} | {'MEMORY (KB)':<12}"
    print(BOLD + header + RESET)
    print("-" * len(header))

def benchmark():
    if not os.path.exists("../build/generator"):
        print(f"{RED}Error: Build binaries not found in ../build/. Run make first!{RESET}")
        return

    for i in range(ITERATIONS):
        print(f"{BOLD}Round {i+1}/{ITERATIONS} (Seed: {i}){RESET}")
        
        with open("test.in", "w") as f:
            subprocess.run(["../build/generator", str(N), str(M), str(i)], stdout=f, check=True)

        run_command(["../build/decremental_tree_connectivity", BASE_ALGO], "test.in")

        ref_out = None
        ref_time = float('inf')
        ref_mem = 0
        for _ in range(3):
            out, duration, mem, _ = run_command(["../build/decremental_tree_connectivity", BASE_ALGO], "test.in")
            if duration < ref_time:
                ref_time = duration
                ref_out = out
                ref_mem = mem
        
        print(f"{GREEN}{BASE_ALGO:<20}{RESET} | {GREEN}BASELINE{RESET}   | {ref_time:>9.2f} | {ref_mem:>11}")

        current_algos = list(ALGORITHMS)
        random.shuffle(current_algos)

        for algo in current_algos:
            best_duration = float('inf')
            best_mem = 0
            best_out = ""
            final_rc = 0

            for _ in range(3):
                out, duration, mem, rc = run_command(["../build/decremental_tree_connectivity", algo], "test.in")
                if rc != 0:
                    final_rc = rc
                    break
                if duration < best_duration:
                    best_duration = duration
                    best_mem = mem
                    best_out = out
            
            if final_rc != 0:
                status = f"{RED}CRASHED{RESET}"
            elif best_out == ref_out:
                status = f"{GREEN}PASSED{RESET}"
            else:
                status = f"{RED}WRONG{RESET}"
                with open("test1.out", "w") as f1, open("test2.out", "w") as f2:
                    f1.write(ref_out)
                    f2.write(best_out)
            
            print(f"{BOLD}{algo:<20}{RESET} | {status:<19} | {best_duration:>9.2f} | {best_mem:>11}")
        print("-" * 60)

if __name__ == "__main__":
    print_header()
    benchmark()