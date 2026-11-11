"""Independent full-set checks for maximality, boundaries and width transitions."""
from pathlib import Path
import itertools
import os
import random
import struct
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from common import BUILD, check_configuration, command, measurement, source_identity


def random_graph(n, seed, density):
    rng = random.Random(seed)
    graph = [set() for _ in range(n)]
    for u, v in itertools.combinations(range(n), 2):
        if rng.random() < density:
            graph[u].add(v)
            graph[v].add(u)
    return graph


def write_graph(path, graph):
    with path.open('wb') as handle:
        handle.write(struct.pack('<II', len(graph), sum(map(len, graph))))
        handle.write(struct.pack('<' + 'I' * len(graph), *map(len, graph)))
        for row in graph:
            if row:
                handle.write(struct.pack('<' + 'I' * len(row), *sorted(row)))


def oracle(graph, k, q):
    n = len(graph)
    adjacency = [sum(1 << v for v in row) for row in graph]
    def valid(mask):
        return all((mask & adjacency[v]).bit_count() + k >= mask.bit_count()
                   for v in range(n) if mask >> v & 1)
    return {tuple(v for v in range(n) if mask >> v & 1)
            for mask in range(1 << n) if mask.bit_count() >= q and valid(mask)
            and not any(valid(mask | (1 << v)) for v in range(n) if not mask >> v & 1)}


def complement_stars(fixed, leaves):
    n = fixed + 2 * (leaves + 1)
    graph = [set(range(n)) - {v} for v in range(n)]
    options = []
    for center in [fixed, fixed + leaves + 1]:
        vertices = set(range(center + 1, center + leaves + 1))
        options.append([vertices] + [{center, v} for v in sorted(vertices)])
        for v in vertices:
            graph[center].remove(v)
            graph[v].remove(center)
    expected = {tuple(sorted(set(range(fixed)) | a | b)) for a in options[0] for b in options[1]}
    return graph, expected


class ExactEnumeration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (BUILD / 'verify/bin/Sequential').is_file():
            raise RuntimeError('Build verification binaries first: bash build.sh --verify')
        cls.identity = source_identity()

    def check_graph(self, graph, k, q, expected, modes=None, allow_fewer_workers=True):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'graph.bin'
            write_graph(path, graph)
            for mode, threads in modes or [('sequential', 1), ('parallel', 1), ('parallel', 4)]:
                with self.subTest(n=len(graph), k=k, q=q, mode=mode, threads=threads):
                    env = {**os.environ, 'CORE_VERIFY_SETS': '1'}
                    result = subprocess.run(command(path, k, q, threads, mode, verify=True),
                                            env=env, capture_output=True, text=True, timeout=120)
                    self.assertEqual(result.returncode, 0, result.stderr + result.stdout[-1000:])
                    found = [tuple(map(int, line.split()[1:])) for line in result.stdout.splitlines()
                             if line.startswith('VERIFY_SET ')]
                    self.assertEqual(len(found), len(set(found)), 'duplicate output')
                    self.assertEqual(set(found), expected)
                    self.assertEqual(measurement(result.stdout, k)[0], len(expected))
                    check_configuration(result.stdout, mode, threads, self.identity, k, q,
                                        allow_fewer_workers=allow_fewer_workers)

    def test_random_graphs_against_independent_oracle(self):
        for n, seed, density, k, q in [(12, 11, .72, 2, 5), (14, 14, .78, 3, 7),
                                       (15, 17, .72, 4, 9), (12, 20, .98, 5, 10),
                                       (10, 23, .15, 2, 5), (16, 26, .78, 3, 12)]:
            graph = random_graph(n, seed, density)
            self.check_graph(graph, k, q, oracle(graph, k, q))

    def test_exact_q_boundary_and_lower_input_rejection(self):
        for k, n, density in [(2, 8, .45), (3, 10, .58), (4, 12, .68), (5, 13, .60)]:
            graph = random_graph(n, 20261025 + k, density)
            q = 2 * k - 1
            expected = oracle(graph, k, q)
            self.assertTrue(any(len(solution) == q for solution in expected))
            self.check_graph(graph, k, q, expected)
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'graph.bin'
            write_graph(path, random_graph(8, 1, .5))
            for frontend in ['Sequential', 'Parallel']:
                result = subprocess.run([str(BUILD / 'verify/bin' / frontend), str(path),
                                         '-k', '3', '-q', '4'], capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertIn('q >= 2*k-1', result.stderr)

    def test_original_ids_survive_preprocessing(self):
        graph = [set() for _ in range(4)] + [{v + 4 for v in row} for row in random_graph(13, 7, .8)]
        self.check_graph(graph, 3, 8, oracle(graph, 3, 8))

    def test_boundary_tight_two_hop_witnesses(self):
        # Two K_k cliques sharing one vertex form a (2k-1)-vertex k-plex.
        # Nonadjacent vertices in different cliques have exactly one common neighbor.
        for k in [2, 3, 4, 5, 8, 9, 17]:
            n = 2 * k - 1
            graph = [set() for _ in range(n)]
            for clique in [range(k), [0] + list(range(k, n))]:
                for u, v in itertools.combinations(clique, 2):
                    graph[u].add(v)
                    graph[v].add(u)
            self.check_graph(graph, k, n, {tuple(range(n))})
            self.check_graph(graph, k, n + 1, set())

    def test_exhaustive_four_vertex_boundary(self):
        # Exhaust every labeled simple graph, rather than sampling one density.
        pairs = list(itertools.combinations(range(4), 2))
        for mask in range(1 << len(pairs)):
            graph = [set() for _ in range(4)]
            for bit, (u, v) in enumerate(pairs):
                if mask >> bit & 1:
                    graph[u].add(v)
                    graph[v].add(u)
            for q in [3, 4]:
                self.check_graph(graph, 2, q, oracle(graph, 2, q))

    def test_boundary_and_nearby_threshold_matrix(self):
        for k in range(2, 7):
            n = 2 * k + 2
            for seed, density in [(31, .25), (37, .55), (41, .8), (43, 1.0)]:
                graph = random_graph(n, seed + k, density)
                for q in [2 * k - 1, 2 * k, 2 * k + 1]:
                    self.check_graph(graph, k, q, oracle(graph, k, q))

    def test_boundary_empty_and_fully_peeled_graphs(self):
        for graph, k in [([], 2), ([set()], 2), ([set() for _ in range(9)], 3),
                         ([{1}, {0}, {3}, {2}], 2)]:
            self.check_graph(graph, k, 2 * k - 1, set())

    def test_boundary_with_fixed_vertices_and_wide_domains(self):
        # Same known maximal sets at the lowest supported q, including >256 vertices.
        for fixed, leaves in [(8, 5), (24, 4), (26, 5), (60, 6), (125, 5), (270, 4)]:
            graph, expected = complement_stars(fixed, leaves)
            self.check_graph(graph, 2, 3, expected)

    def test_lower_boundary_rejected_by_both_frontends(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'graph.bin'
            write_graph(path, random_graph(8, 1, .5))
            for k in [2, 3, 4, 5, 9, 17]:
                for frontend in ['Sequential', 'Parallel']:
                    result = subprocess.run([str(BUILD / 'verify/bin' / frontend), str(path),
                                             '-k', str(k), '-q', str(2 * k - 2)],
                                            capture_output=True, text=True)
                    self.assertEqual(result.returncode, 2)
                    self.assertIn('q >= 2*k-1', result.stderr)

    def test_fixed_vertices_and_wide_domains(self):
        for fixed, leaves in [(8, 5), (24, 4), (26, 5), (60, 6), (125, 5), (270, 4)]:
            graph, expected = complement_stars(fixed, leaves)
            self.check_graph(graph, 2, fixed + 4, expected)

    def test_sixteen_worker_correctness(self):
        graph, expected = complement_stars(8, 5)
        self.check_graph(graph, 2, 12, expected, [('parallel', 16)], allow_fewer_workers=False)

    def test_k_above_fast_specializations(self):
        graph = random_graph(19, 3, .98)
        self.assertTrue(all(len(row) + 9 >= len(graph) for row in graph))
        self.check_graph(graph, 9, 18, {tuple(range(len(graph)))})


if __name__ == '__main__':
    unittest.main()
