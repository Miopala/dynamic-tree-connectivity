# Performance

Baseline before optimization.

## Setup

- `standard` suite: 36 cases (9 tree topologies × 4 workloads), 100000 nodes and operations, 8 rounds per case
- GCC 14.2.0 with `-O3`
- Intel Core i7-9850H under WSL2

```bash
python3 scripts/benchmark.py standard --verbose
```

## Results

### Relative total time

Each algorithm is compared with the fastest algorithm in the same case. A
slowdown of `2.0x` means that it took twice as long. `Wins` is the number of
cases in which the algorithm was fastest. Total time includes preprocessing
and operations.

| Algorithm | Wins | Geomean slowdown | Median slowdown | Worst slowdown |
|---|---:|---:|---:|---:|
| `micro-trees` | 36 | 1.000x | 1.000x | 1.000x |
| `small-to-large` | 0 | 2.440x | 2.610x | 4.105x |
| `euler-tour` | 0 | 2.346x | 2.356x | 3.410x |
| `euler-tour-trees` | 0 | 7.362x | 7.357x | 11.294x |

### Median timings

For each case, the median time is calculated from 8 rounds. The table reports
the median result across all 36 cases. Each column is summarized separately,
so the values may not add up exactly.

| Algorithm | Preprocessing [ms] | Operations [ms] | Total [ms] |
|---|---:|---:|---:|
| `micro-trees` | 34.752 | 8.152 | 44.320 |
| `small-to-large` | 40.271 | 76.543 | 115.362 |
| `euler-tour` | 60.731 | 39.207 | 97.841 |
| `euler-tour-trees` | 179.956 | 152.050 | 328.678 |

## Small-to-large ordered cuts

Deleted edges previously stayed in adjacency lists, causing quadratic behavior
on ordered star cuts. They are now removed with swap-and-pop during traversal.
Each result is the median operation time across 4 rounds of the
`small-to-large-adversarial` suite.

| Nodes | Before [ms] | After [ms] | Speedup |
|---:|---:|---:|---:|
| 4000 | 22.867 | 0.724 | 31.6x |
| 8000 | 90.612 | 1.371 | 66.1x |
| 16000 | 359.265 | 2.815 | 127.6x |
| 100000 | 15816.477 | 34.277 | 461.4x |
