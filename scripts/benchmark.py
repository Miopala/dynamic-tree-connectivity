import os
import statistics
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class WorkloadProfile:
    name: str
    cut_ratio: float
    same_component_heavy: bool
    same_component_ratio: float
    ordered_cuts: bool = False


@dataclass(frozen=True)
class BenchmarkCase:
    name: str
    node_count: int
    operation_count: int
    seed: int
    tree_type: str
    workload: WorkloadProfile


@dataclass(frozen=True)
class BenchmarkSuite:
    rounds: int
    cases: tuple[BenchmarkCase, ...]


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = PROJECT_ROOT / "build"

GENERATOR = BUILD_DIR / "generator"
SOLVER = BUILD_DIR / "decremental_tree_connectivity"
BENCHMARK_INPUT = BUILD_DIR / "benchmark.in"
TIMEOUT_SECONDS = 120

COLOR_ENABLED = sys.stdout.isatty() and "NO_COLOR" not in os.environ
RESET = "\033[0m"
BOLD = "\033[1m"
CYAN = "\033[36m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
MAGENTA = "\033[35m"

ALGORITHMS = [
    "micro-trees",
    "small-to-large",
    "euler-tour",
    "euler-tour-trees",
]

TREE_TYPES = (
    "random",
    "line",
    "star",
    "balanced_binary",
    "deep",
    "caterpillar",
    "caterpillar_with_trees",
    "misc_1",
    "misc_2",
)

QUERY_HEAVY_WORKLOAD = WorkloadProfile(
    name="query-heavy",
    cut_ratio=0.1,
    same_component_heavy=False,
    same_component_ratio=0.0,
)

MIXED_WORKLOAD = WorkloadProfile(
    name="mixed",
    cut_ratio=0.5,
    same_component_heavy=False,
    same_component_ratio=0.0,
)

SAME_COMPONENT_HEAVY_WORKLOAD = WorkloadProfile(
    name="same-component-heavy",
    cut_ratio=0.5,
    same_component_heavy=True,
    same_component_ratio=0.8,
)

CUT_HEAVY_WORKLOAD = WorkloadProfile(
    name="cut-heavy",
    cut_ratio=0.8,
    same_component_heavy=False,
    same_component_ratio=0.0,
)

ORDERED_CUTS_WORKLOAD = WorkloadProfile(
    name="ordered-cuts",
    cut_ratio=1.0,
    same_component_heavy=False,
    same_component_ratio=0.0,
    ordered_cuts=True,
)

WORKLOAD_PROFILES = (
    QUERY_HEAVY_WORKLOAD,
    MIXED_WORKLOAD,
    SAME_COMPONENT_HEAVY_WORKLOAD,
    CUT_HEAVY_WORKLOAD,
)


def make_benchmark_cases(
    scale_name,
    node_count,
    operation_count,
    seeds,
    tree_types=TREE_TYPES,
    workload_profiles=WORKLOAD_PROFILES,
):
    return tuple(
        BenchmarkCase(
            name=f"{scale_name}-{tree_type}-{workload.name}-seed-{seed}",
            node_count=node_count,
            operation_count=operation_count,
            seed=seed,
            tree_type=tree_type,
            workload=workload,
        )
        for seed in seeds
        for tree_type in tree_types
        for workload in workload_profiles
    )


BENCHMARK_SUITES = {
    "quick": BenchmarkSuite(
        rounds=4,
        cases=make_benchmark_cases(
            "small",
            10000,
            10000,
            (12345,),
        ),
    ),
    "standard": BenchmarkSuite(
        rounds=8,
        cases=make_benchmark_cases(
            "medium",
            100000,
            100000,
            (12345,),
        ),
    ),
    "full": BenchmarkSuite(
        rounds=8,
        cases=(
            make_benchmark_cases(
                "medium",
                100000,
                100000,
                (12345, 67890, 314159),
            )
            + make_benchmark_cases(
                "large",
                300000,
                300000,
                (12345,),
                workload_profiles=(MIXED_WORKLOAD,),
            )
            + make_benchmark_cases(
                "large",
                300000,
                300000,
                (12345,),
                tree_types=("random",),
                workload_profiles=(
                    QUERY_HEAVY_WORKLOAD,
                    SAME_COMPONENT_HEAVY_WORKLOAD,
                    CUT_HEAVY_WORKLOAD,
                ),
            )
            + make_benchmark_cases(
                "extra-large",
                1000000,
                1000000,
                (12345,),
            )
        ),
    ),
    "extreme": BenchmarkSuite(
        rounds=4,
        cases=make_benchmark_cases(
            "extreme",
            10000000,
            10000000,
            (12345,),
        ),
    ),
    "small-to-large-adversarial": BenchmarkSuite(
        rounds=4,
        cases=tuple(
            BenchmarkCase(
                name=f"ordered-star-cuts-{node_count}",
                node_count=node_count,
                operation_count=node_count - 1,
                seed=12345,
                tree_type="star",
                workload=ORDERED_CUTS_WORKLOAD,
            )
            for node_count in (4000, 8000, 16000, 100000)
        ),
    ),
}


# Balanced Williams order
BALANCED_ORDER_OFFSETS = [0, 1, 3, 2]


def colorize(text, *styles):
    if not COLOR_ENABLED:
        return text

    return f"{''.join(styles)}{text}{RESET}"


def algorithm_order(round_index):
    algorithm_count = len(ALGORITHMS)

    return [
        ALGORITHMS[(offset + round_index) % algorithm_count]
        for offset in BALANCED_ORDER_OFFSETS
    ]


def check_benchmark_configuration(round_count):
    algorithm_count = len(ALGORITHMS)

    if (
        algorithm_count == 0
        or sorted(BALANCED_ORDER_OFFSETS) != list(range(algorithm_count))
        or round_count < algorithm_count
        or round_count % algorithm_count != 0
    ):
        raise RuntimeError("Invalid balanced benchmark configuration!")


def check_binaries():
    for binary in (GENERATOR, SOLVER):
        if not binary.is_file():
            raise RuntimeError(f"Missing binary: {binary}!")


def generate_input(benchmark_case):
    with BENCHMARK_INPUT.open("w") as output_file:
        try:
            result = subprocess.run(
                [
                    GENERATOR,
                    str(benchmark_case.node_count),
                    str(benchmark_case.operation_count),
                    str(benchmark_case.seed),
                    benchmark_case.tree_type,
                    str(benchmark_case.workload.cut_ratio),
                    "1" if benchmark_case.workload.same_component_heavy else "0",
                    str(benchmark_case.workload.same_component_ratio),
                    "1" if benchmark_case.workload.ordered_cuts else "0",
                ],
                stdout=output_file,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
                timeout=TIMEOUT_SECONDS,
            )
        except subprocess.TimeoutExpired as error:
            raise RuntimeError(
                f"Generator timed out after {TIMEOUT_SECONDS} seconds."
            ) from error

    if result.returncode != 0:
        raise RuntimeError(
            f"Generator failed with exit code {result.returncode}:\n"
            f"{result.stderr}"
        )


def run_algorithm(algorithm):
    with BENCHMARK_INPUT.open("r") as input_file:
        try:
            result = subprocess.run(
                [SOLVER, algorithm, "--benchmark"],
                stdin=input_file,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
                timeout=TIMEOUT_SECONDS,
            )
        except subprocess.TimeoutExpired as error:
            raise RuntimeError(
                f"{algorithm} timed out after {TIMEOUT_SECONDS} seconds"
            ) from error

    if result.returncode != 0:
        raise RuntimeError(
            f"{algorithm} failed with exit code {result.returncode}:\n"
            f"{result.stderr}"
        )

    output_values = result.stdout.split()

    if len(output_values) != 3:
        raise RuntimeError(
            f"{algorithm} returned benchmark output in invalid format: "
            f"{result.stdout!r}"
        )

    preprocessing_ns, operations_ns, checksum = map(int, output_values)

    return preprocessing_ns, operations_ns, checksum


def run_warmup():
    expected_checksum = None

    for algorithm in ALGORITHMS:
        _, _, checksum = run_algorithm(algorithm)

        if expected_checksum is None:
            expected_checksum = checksum
        elif checksum != expected_checksum:
            raise RuntimeError(
                f"Warm-up checksum mismatch for {algorithm}: "
                f"expected {expected_checksum}, got {checksum}"
            )

    return expected_checksum


def print_summary(timings_by_algorithm):
    sample_count = len(timings_by_algorithm[ALGORITHMS[0]])

    print(colorize(f"Benchmark summary ({sample_count} samples)", BOLD, CYAN))
    header = (
        f"{'Algorithm':<20}"
        f"{'Phase':<16}"
        f"{'Min [ms]':>12}"
        f"{'Median [ms]':>14}"
        f"{'Max [ms]':>12}"
        f"{'Std dev [ms]':>16}"
    )
    print(colorize(header, BOLD))

    for algorithm in ALGORITHMS:
        timings = timings_by_algorithm[algorithm]

        timings_by_phase = {
            "Preprocessing": [
                preprocessing_ns
                for preprocessing_ns, _ in timings
            ],
            "Operations": [
                operations_ns
                for _, operations_ns in timings
            ],
            "Total": [
                preprocessing_ns + operations_ns
                for preprocessing_ns, operations_ns in timings
            ],
        }

        for phase, phase_timings in timings_by_phase.items():
            minimum_ns = min(phase_timings)
            median_ns = statistics.median(phase_timings)
            maximum_ns = max(phase_timings)
            standard_deviation_ns = statistics.stdev(phase_timings)

            print(
                colorize(f"{algorithm:<20}", BOLD)
                + f"{phase:<16}"
                + colorize(f"{minimum_ns / 1000000:>12.3f}", GREEN)
                + colorize(f"{median_ns / 1000000:>14.3f}", CYAN)
                + colorize(f"{maximum_ns / 1000000:>12.3f}", YELLOW)
                + colorize(
                    f"{standard_deviation_ns / 1000000:>16.3f}",
                    MAGENTA,
                )
            )

    print()


def print_suite_summary(median_total_timings_by_case):
    wins_by_algorithm = {
        algorithm: 0
        for algorithm in ALGORITHMS
    }
    slowdowns_by_algorithm = {
        algorithm: []
        for algorithm in ALGORITHMS
    }

    for median_total_timings in median_total_timings_by_case:
        fastest_timing = min(median_total_timings.values())

        for algorithm, timing in median_total_timings.items():
            if timing == fastest_timing:
                wins_by_algorithm[algorithm] += 1

            slowdowns_by_algorithm[algorithm].append(
                timing / fastest_timing
            )

    print(
        colorize(
            f"Suite summary ({len(median_total_timings_by_case)} cases)",
            BOLD,
            CYAN,
        )
    )
    print("Slowdowns are relative to the fastest median total time in each case.")
    header = (
        f"{'Algorithm':<20}"
        f"{'Wins':>8}"
        f"{'Geomean slowdown':>20}"
        f"{'Median slowdown':>18}"
        f"{'Worst slowdown':>17}"
    )
    print(colorize(header, BOLD))

    for algorithm in ALGORITHMS:
        slowdowns = slowdowns_by_algorithm[algorithm]
        geometric_mean = statistics.geometric_mean(slowdowns)
        median_slowdown = statistics.median(slowdowns)
        worst_slowdown = max(slowdowns)

        print(
            colorize(f"{algorithm:<20}", BOLD)
            + colorize(f"{wins_by_algorithm[algorithm]:>8}", GREEN)
            + colorize(f"{geometric_mean:>19.3f}x", CYAN)
            + f"{median_slowdown:>17.3f}x"
            + colorize(f"{worst_slowdown:>16.3f}x", YELLOW)
        )

    print()


def print_configuration(benchmark_case, round_count):
    same_component_setting = (
        benchmark_case.workload.same_component_ratio
        if benchmark_case.workload.same_component_heavy
        else "disabled"
    )

    print(
        f"Nodes: {benchmark_case.node_count} | "
        f"Operations: {benchmark_case.operation_count} | "
        f"Seed: {benchmark_case.seed} | "
        f"Rounds: {round_count}"
    )
    print(
        f"Tree: {benchmark_case.tree_type} | "
        f"Workload: {benchmark_case.workload.name} | "
        f"Cut ratio: {benchmark_case.workload.cut_ratio} | "
        f"Same-component ratio: {same_component_setting}"
    )


def run_benchmark_case(benchmark_case, round_count, verbose):
    if verbose:
        print_configuration(benchmark_case, round_count)

    if verbose:
        print("Generating input ... ", end="", flush=True)
    generate_input(benchmark_case)
    if verbose:
        print(colorize("completed", GREEN))

    if verbose:
        print("Running warm-up ... ", end="", flush=True)
    expected_checksum = run_warmup()
    if verbose:
        print(colorize("completed", GREEN))

    timings_by_algorithm = {
        algorithm: []
        for algorithm in ALGORITHMS
    }

    for round_index in range(round_count):
        current_order = algorithm_order(round_index)

        if verbose:
            print(colorize(f"Round {round_index + 1}/{round_count}", BOLD, CYAN))
            header = (
                f"{'Algorithm':<20}"
                f"{'Preprocessing [ms]':>20}"
                f"{'Operations [ms]':>18}"
                f"{'Total [ms]':>14}"
            )
            print(colorize(header, BOLD))

        for algorithm in current_order:
            preprocessing_ns, operations_ns, checksum = run_algorithm(
                algorithm
            )

            if checksum != expected_checksum:
                raise RuntimeError(
                    f"Checksum mismatch for {algorithm}: "
                    f"expected {expected_checksum}, got {checksum}"
                )

            timings_by_algorithm[algorithm].append(
                (preprocessing_ns, operations_ns)
            )

            if verbose:
                print(
                    colorize(f"{algorithm:<20}", BOLD)
                    + f"{preprocessing_ns / 1000000:>20.3f}"
                    + f"{operations_ns / 1000000:>18.3f}"
                    + colorize(
                        f"{(preprocessing_ns + operations_ns) / 1000000:>14.3f}",
                        CYAN,
                    )
                )

        if verbose:
            print()
        else:
            print(colorize(".", CYAN), end="", flush=True)

    if not verbose:
        print(f"] {colorize('completed', GREEN)}")

    median_total_timings = {
        algorithm: statistics.median(
            preprocessing_ns + operations_ns
            for preprocessing_ns, operations_ns in timings
        )
        for algorithm, timings in timings_by_algorithm.items()
    }

    if verbose:
        print_summary(timings_by_algorithm)

    return median_total_timings


def run_benchmark_suite(benchmark_suite, verbose):
    check_benchmark_configuration(benchmark_suite.rounds)
    median_total_timings_by_case = []

    for case_number, benchmark_case in enumerate(
        benchmark_suite.cases,
        start=1,
    ):
        if verbose:
            print(
                colorize(
                    f"=== Case {case_number}/{len(benchmark_suite.cases)}: "
                    f"{benchmark_case.name} ===",
                    BOLD,
                    CYAN,
                )
            )
        else:
            print(
                colorize(
                    f"[{case_number}/{len(benchmark_suite.cases)}] "
                    f"{benchmark_case.name}",
                    BOLD,
                    CYAN,
                ),
                end=" [",
                flush=True,
            )

        median_total_timings = run_benchmark_case(
            benchmark_case,
            benchmark_suite.rounds,
            verbose,
        )
        median_total_timings_by_case.append(median_total_timings)

    print_suite_summary(median_total_timings_by_case)


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3):
        raise SystemExit(
            f"Usage: {Path(sys.argv[0]).name} <suite> [--verbose]"
        )

    suite_name = sys.argv[1]

    if len(sys.argv) == 3 and sys.argv[2] != "--verbose":
        raise SystemExit(
            f"Unknown option: {sys.argv[2]}. Available option: --verbose"
        )

    verbose = len(sys.argv) == 3

    if suite_name not in BENCHMARK_SUITES:
        available_suites = ", ".join(BENCHMARK_SUITES)
        raise SystemExit(
            f"Unknown benchmark suite: {suite_name}. "
            f"Available suites: {available_suites}"
        )

    check_binaries()
    run_benchmark_suite(BENCHMARK_SUITES[suite_name], verbose)
