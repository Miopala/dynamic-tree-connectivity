# Decremental Online Tree Connectivity

A C++ project for solving decremental online tree connectivity problem.

The main implementation is based on the microtree-macrotree decomposition based on the paper by Alstrup, Secher, and Spork from 1997. The project also contains several
baseline algorithms, including Euler-tour trees, euler-tour approach, brute-force, and
small-to-large component relabeling.


## Problem

Given an undirected tree with `n` vertices, process an online sequence of following operations:

- `cut(u, v)` - delete the edge between `u` and `v`.
- `connected(u, v)` - check if `u` and `v` are in the same connected component.

The graph is decremental, which means, we can only delete edges from the tree.

## Implemented algorithms

| Algorithm | Approach | Preprocessing | `cut` | `connected` | Space |
|---|---|---:|---:|---:|---:|
| `brute-force` | adjacency lists and BFS | O(n) | O(n) | O(n) | O(n) |
| `euler-tour` | static Euler tour, LCA and Fenwick tree | O(n log n) | O(log n) | O(log n) | O(n log n) |
| `small-to-large` | component relabeling on the smaller side | O(n) | O(log n) amortized | O(1) | O(n) |
| `euler-tour-trees` | Euler-tour sequences represented by splay trees | O(n log n) amortized | O(log n) amortized | O(log n) amortized | O(n) |
| `micro-trees` | microtree-macrotree decomposition with bitmasks | O(n) | O(1) amortized* | O(1)* | O(n) |

The bound marked with `*` in microtree-macrotree decomposition approach assumes word-RAM model, where a word contains at least Ω(log n) bits and bitwise operations on numbers of word size take O(1) time.

Euler-tour trees support also solving the version with inserting edges.



## Input format
```
n m
u1 v1
u2 v2
...
u(n-1) v(n-1)
operation1 u v
operation2 u v
...
operationm u v
```

Vertices are numbered from `0` to `n - 1`.

Operations:

- `C u v` - cut edge `(u, v)`.
- `Q u v` - query whether `u` and `v` are connected.

Example:

```text
5 5
0 1
1 2
1 3
3 4
Q 0 4
C 1 3
Q 0 4
Q 3 4
Q 2 4
```

Run an implementation with:

```bash
./build/decremental_tree_connectivity micro-trees < input.txt
```

Available algorithm names:

```text
brute-force
euler-tour
small-to-large
euler-tour-trees
micro-trees
```

The program prints `YES` or `NO` for every connectivity query.

## Test generator

The generator supports several tree topologies:

| Tree type | Construction |
|---|---|
| `random` | Random recursive tree: every vertex `i` chooses its parent uniformly from vertices `[0, i - 1]`. |
| `star` | One central vertex connected directly to every other vertex. |
| `line` | A single path containing all vertices. |
| `balanced_binary` | A nearly balanced binary tree in which vertex `i` is connected to vertex `floor(i / 2)`. |
| `deep` | A deep, path-like random tree: vertex `i` connects between 1 and 100 positions backwards. |
| `caterpillar` | A path of approximately `n / log2(n)` spine vertices, with every remaining vertex attached directly to a random spine vertex. |
| `caterpillar_with_trees` | The same initial spine, but remaining vertices attach to arbitrary earlier vertices, producing random subtrees around the spine. |
| `misc_1` | A path-like first half with short backward jumps, followed by a recursively grown region concentrated near the end of that half. |
| `misc_2` | A root with approximately `sqrt(n)` children, from which hang some straight lines with noise |

After constructing the topology, the generator randomly relabels the vertices, shuffles the edge order, and randomly reverses edge orientations.

Operations are then generated using `cut_ratio` as the probability of selecting the next previously uncut edge for deletion. Other operations are connectivity queries between random vertices. When `same_component_heavy` is enabled, the generator attempts to make 80% of those queries ask about two vertices that are connected at that point in the operation sequence.

Example:

```bash
./build/generator 1000 5000 12345 random > test.in
./build/decremental_tree_connectivity micro-trees < test.in
```

Generator arguments:

```text
generator <nodes> <operations> <seed> <tree_type> [cut_ratio] [same_component_heavy]
```

## References

1. Stephen Alstrup, Jens Peter Secher, and Maz Spork. “Optimal On-Line Decremental Connectivity in Trees,” 1997. [DOI: 10.1016/S0020-0190(97)00170-1](https://doi.org/10.1016/S0020-0190(97)00170-1).

2. Erik Demaine. “Dynamic Graphs and Euler-Tour Trees.” Lecture 20, *MIT 6.851: Advanced Data Structures*, 2012. [Lecture notes](https://ocw.mit.edu/courses/6-851-advanced-data-structures-spring-2012/resources/mit6_851s12_lec20/).
