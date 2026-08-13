import subprocess
import unittest
from pathlib import Path
import random


ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "build" / "generator"

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


def run_generator(node_count, query_count, seed, tree_type):
    return subprocess.run(
        [
            str(GENERATOR),
            str(node_count),
            str(query_count),
            str(seed),
            tree_type,
        ],
        capture_output=True,
        text=True,
        check=False,
    )

def parse_output(output):
        lines = output.splitlines()

        if not lines:
            raise ValueError("Empty generator output")

        size_fields = lines[0].split()
        if len(size_fields) != 2:
            raise ValueError("First line must contain exactly two values")

        node_count, query_count = map(int, size_fields)

        if node_count < 1 or query_count < 0:
            raise ValueError("Invalid node or query count")

        expected_lines = 1 + (node_count - 1) + query_count
        if len(lines) != expected_lines:
            raise ValueError(
                f"Expected {expected_lines} lines, got {len(lines)}"
            )

        edges = []

        for line in lines[1:node_count]:
            fields = line.split()

            if len(fields) != 2:
                raise ValueError(f"Invalid edge format, line: {line}")

            u, v = map(int, fields)

            if (min(u,v) < 0 or max(u,v) >= node_count):
                raise ValueError(f"u or v is invalid in: ({u} {v})")

            edges.append((u, v))

        operations = []

        for line in lines[node_count:]:
            fields = line.split()

            if len(fields) != 3:
                raise ValueError(f"Invalid operation format, line: {line}")

            operation, u, v = fields

            u, v = map(int, (u, v))


            if operation not in {"C", "Q"}:
                raise ValueError(f"Unknown operation: {operation}")

            if (min(u,v) < 0 or max(u,v) >= node_count):
                raise ValueError(f"u or v is invalid in: ({u} {v})")
            

            operations.append((operation, int(u), int(v)))

        return node_count, query_count, edges, operations


class TestGenerator(unittest.TestCase):

    def test_basic_output(self):
        tests_number = 100
        rng = random.Random(10000)

        for test_number in range(tests_number):
            node_count = rng.randint(1, 100)
            query_count = rng.randint(0, 500)
            generator_seed = rng.randint(0, 1000000)

            for tree_type in TREE_TYPES:
                with self.subTest(
                    test_number=test_number,
                    tree_type=tree_type,
                    node_count=node_count,
                    query_count=query_count,
                    seed=generator_seed,
                ):
                    result = run_generator(
                        node_count,
                        query_count,
                        generator_seed,
                        tree_type,
                    )

                    self.assertEqual(result.returncode, 0, result.stderr)
                    node_count_input, query_count_input, edges, operations = parse_output(result.stdout)

                    self.assertEqual(node_count, node_count_input)          
                    self.assertEqual(query_count, query_count_input)

                    neighbors = [[] for _ in range(node_count)]
                    tree_edges = set()

                    for u, v in edges:
                        self.assertNotEqual(u, v)

                        edge = tuple(sorted((u, v)))
                        self.assertNotIn(edge, tree_edges)
                        tree_edges.add(edge)

                        neighbors[u].append(v)
                        neighbors[v].append(u)

                    visited = set()
                    stack = [0]

                    while stack:
                        u = stack.pop()
                        if u in visited:
                            continue

                        visited.add(u)
                        stack.extend(neighbors[u])

                    self.assertEqual(len(visited), node_count)

                    active_edges = set(tree_edges)

                    for operation, u, v in operations:
                        if operation == "C":
                            edge = tuple(sorted((u, v)))
                            self.assertIn(edge, active_edges)
                            active_edges.remove(edge)



    def test_seed_reliability(self):
        tests_number = 100
        rng = random.Random(20000)

        for test_number in range(tests_number):
            node_count = rng.randint(1, 100)
            query_count = rng.randint(0, 500)
            generator_seed = rng.randint(0, 1000000)

            for tree_type in TREE_TYPES:
                with self.subTest(
                    test_number=test_number,
                    tree_type=tree_type,
                    node_count=node_count,
                    query_count=query_count,
                    seed=generator_seed,
                ):
                    first = run_generator(
                        node_count,
                        query_count,
                        generator_seed,
                        tree_type,
                    )
                    second = run_generator(
                        node_count,
                        query_count,
                        generator_seed,
                        tree_type,
                    )

                    self.assertEqual(first.returncode, 0, first.stderr)
                    self.assertEqual(second.returncode, 0, second.stderr)
                    self.assertEqual(first.stdout, second.stdout)


if __name__ == "__main__":
    unittest.main()
