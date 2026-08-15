import subprocess
from pathlib import Path
import sys
import random
import resource

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
ALGORITHMS = ["micro-trees"]
BASE_ALGO = "small-to-large"
BRUTE_ALGO = "brute-force"
SOLVER_TIMEOUT_SECONDS = 60


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = PROJECT_ROOT / "build"
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
        n, m, ref_algo = 100, 50000, BRUTE_ALGO
        test_algos = ALGORITHMS + [BASE_ALGO]
    elif mode == "stress_test":
        n, m, ref_algo = 1_000_000, 2_000_000, BASE_ALGO
        test_algos = ALGORITHMS

    else:
        print(
            f"{RED}Unknown mode: {mode}. "
            f"Expected correctness_test or stress_test.{RESET}"
        )
        sys.exit(2)

    for i in range(1, iterations + 1):
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
                    f"REFERENCE TIMEOUT (STOPS){RESET}"
                )
                sys.exit(1)


            if reference.returncode != 0:
                failure_input = BUILD_DIR / "wrong_test.in"
                failure_input.write_text(test_in.read_text())

                reference_stderr = BUILD_DIR / "reference.stderr"
                reference_stderr.write_text(reference.stderr)

                print(f"{RED}{ref_algo:<20} | REFERENCE FAILED (STOPS){RESET}")
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
                        f"{RED}TIMEOUT (STOPS){RESET}"
                    )
                    sys.exit(1)

                if result.returncode != 0:
                    print(f"{BOLD}{algo:<20}{RESET} | {RED}CRASHED (STOPS){RESET}")

                    (BUILD_DIR / "wrong_test.in").write_text(test_in.read_text())
                    (BUILD_DIR / "algo.stderr").write_text(result.stderr)

                    sys.exit(1)

                elif result.stdout == ref_out:
                    print(f"{BOLD}{algo:<20}{RESET} | {GREEN}PASSED{RESET}")

                else:
                    print(f"{BOLD}{algo:<20}{RESET} | {RED}WRONG (STOPS){RESET}")

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
        iterations = int(sys.argv[2]) if len(sys.argv) > 2 else 1
        parent_seed = int(sys.argv[3]) if len(sys.argv) > 3 else 12121
    except ValueError:
        print("Iterations and seed must be integers.", file=sys.stderr)
        print(usage, file=sys.stderr)
        sys.exit(2)

    if iterations < 1:
        print("Iterations must be at least 1.", file=sys.stderr)
        sys.exit(2)

    validate(mode, iterations, parent_seed)
