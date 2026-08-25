import os
import random
import resource
import subprocess
import sys
from pathlib import Path


CORRECTNESS_CASES = [
    (1, 100),
    (2, 100),
    (3, 100),
    (7, 1000),
    (8, 1000),
    (9, 1000),
    (31, 5000),
    (32, 5000),
    (33, 5000),
    (100, 50000),
    (1000, 50000),
]

STRESS_CASES = [
    (1000000, 2000000),
]

TREE_TYPES = [
    "random",
    "star",
    "line",
    "balanced_binary",
    "deep",
    "caterpillar",
    "caterpillar_with_trees",
    "misc_1",
    "misc_2",
]
ALGORITHMS = ["euler-tour", "micro-trees", "euler-tour-trees"]
BASE_ALGO = "small-to-large"
BRUTE_ALGO = "brute-force"
SOLVER_TIMEOUT_SECONDS = 60
DEFAULT_ITERATIONS = 1
DEFAULT_PARENT_SEED = 12121

PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = (
    PROJECT_ROOT / os.environ.get("DYNAMIC_CONNECTIVITY_BUILD_DIR", "build")
).resolve()


GEN = BUILD_DIR / "generator"
SOLVER = BUILD_DIR / "decremental_tree_connectivity"

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

def run_solver(algo, input_file):
    with open(input_file, "r") as test_input:
        return subprocess.run(
            [SOLVER, algo],
            stdin=test_input,
            capture_output=True,
            text=True,
            check=False,
            timeout=SOLVER_TIMEOUT_SECONDS,
        )


def print_header(mode, n, m, iteration, total_iters):
    print(f"\n{BOLD}{BLUE}=== DECREMENTAL CONNECTIVITY {mode.upper()} ==={RESET}")
    print(f"{CYAN}Iteration: {iteration}/{total_iters} | Nodes: {n} | Queries: {m}{RESET}\n")
    header = f"{'ALGORITHM':<20} | {'STATUS':<10}"
    print(BOLD + header + RESET)
    print("-" * len(header))

def validate(mode, iterations, parent_seed):
    rng = random.Random(parent_seed)
    set_unlimited_stack()
    if mode == "correctness_test":
        test_cases = CORRECTNESS_CASES
        ref_algo = BRUTE_ALGO
        test_algos = ALGORITHMS + [BASE_ALGO]

    elif mode == "stress_test":
        test_cases = STRESS_CASES
        ref_algo = BASE_ALGO
        test_algos = ALGORITHMS

    else:
        print(
            f"{RED}Unknown mode: {mode}. "
            f"Expected correctness_test or stress_test.{RESET}"
        )
        sys.exit(2)

    for i in range(1, iterations + 1):
        for n, m in test_cases:
            print_header(mode, n, m, i, iterations)
            for tree_type in TREE_TYPES:
                seed = rng.randint(0, 1000000)
                test_in = BUILD_DIR / "test.in"
                with open(test_in, "w") as f:
                    subprocess.run([GEN, str(n), str(m), str(seed), tree_type], stdout=f, check=True)
                
                print(f"{BOLD}Tree: {tree_type.upper()} (Seed: {seed}){RESET}")
                
                try:
                    reference = run_solver(ref_algo, test_in)
                except subprocess.TimeoutExpired:
                    (BUILD_DIR / "wrong_test.in").write_text(test_in.read_text())

                    print(
                        f"{RED}{ref_algo:<20} | "
                        f"REFERENCE TIMEOUT{RESET}"
                    )
                    sys.exit(1)


                if reference.returncode != 0:
                    failure_input = BUILD_DIR / "wrong_test.in"
                    failure_input.write_text(test_in.read_text())

                    reference_stderr = BUILD_DIR / "reference.stderr"
                    reference_stderr.write_text(reference.stderr)

                    print(f"{RED}{ref_algo:<20} | REFERENCE FAILED{RESET}")
                    sys.exit(1)

                ref_out = reference.stdout
                print(f"{CYAN}{ref_algo:<20}{RESET} | {CYAN}REFERENCE{RESET}")

                for algo in test_algos:
                    try:
                        result = run_solver(algo, test_in)
                    except subprocess.TimeoutExpired:
                        (BUILD_DIR / "wrong_test.in").write_text(test_in.read_text())

                        print(
                            f"{BOLD}{algo:<20}{RESET} | "
                            f"{RED}TIMEOUT{RESET}"
                        )
                        sys.exit(1)

                    if result.returncode != 0:
                        print(f"{BOLD}{algo:<20}{RESET} | {RED}FAILED{RESET}")

                        (BUILD_DIR / "wrong_test.in").write_text(test_in.read_text())
                        (BUILD_DIR / "algo.stderr").write_text(result.stderr)

                        sys.exit(1)

                    elif result.stdout == ref_out:
                        print(f"{BOLD}{algo:<20}{RESET} | {GREEN}PASSED{RESET}")

                    else:
                        print(f"{BOLD}{algo:<20}{RESET} | {RED}WRONG{RESET}")

                        (BUILD_DIR / "wrong_test.in").write_text(test_in.read_text())
                        (BUILD_DIR / "ref.out").write_text(ref_out)
                        (BUILD_DIR / "algo.out").write_text(result.stdout)
                        (BUILD_DIR / "algo.stderr").write_text(result.stderr)

                        sys.exit(1)
                print("-" * 60)

if __name__ == "__main__":
    usage = (
        "Usage: python validate.py "
        "[correctness_test | stress_test] [iterations] [parent_seed]"
    )

    if len(sys.argv) < 2 or len(sys.argv) > 4:
        print(usage, file=sys.stderr)
        sys.exit(2)

    mode = sys.argv[1]

    try:
        iterations = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_ITERATIONS
        parent_seed = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_PARENT_SEED
    except ValueError:
        print("Iterations and seed must be integers.", file=sys.stderr)
        print(usage, file=sys.stderr)
        sys.exit(2)

    if iterations < 1:
        print("Iterations must be at least 1.", file=sys.stderr)
        sys.exit(2)

    validate(mode, iterations, parent_seed)
