from pathlib import Path
import hashlib
import os
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import common
import benchmark


class RunContract(unittest.TestCase):
    def test_reading_a_graph_does_not_invalidate_its_identity(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'graph.bin'
            data = struct.pack('<III', 1, 0, 0)
            path.write_bytes(data)
            os.utime(path, ns=(1, path.stat().st_mtime_ns))
            expected = {'n': 1, 'stored_arcs': 0, 'size': 12,
                        'sha256': hashlib.sha256(data).hexdigest()}
            identity = benchmark.input_identity(path, expected)
            self.assertEqual(identity['sha256'], expected['sha256'])
            benchmark.check_input(path, identity)

    def test_formal_options_and_boundary_are_applied(self):
        args = common.command(Path('/data/g.bin'), 2, 3)
        self.assertIn('--fold-variant', args)
        for flag, value in [('--fold-variant', '3'), ('--selector', 'hybrid4'),
                            ('--local-f-remat', '1'), ('--remat-stack-reuse', '1'),
                            ('--remat-min-slack', '3'), ('--queue-precheck', '1'),
                            ('--remap128', '1'), ('--simd128', '0'), ('-p', '0'), ('-s', '0')]:
            self.assertEqual(args[args.index(flag) + 1], value)
        self.assertTrue(args[0].endswith('/Sequential'))
        args = common.command(Path('/data/g.bin'), 3, 800, 16)
        self.assertTrue(args[0].endswith('/Parallel'))
        self.assertEqual(args[args.index('-t') + 1], '16')

    def test_invalid_problem_or_execution_parameters_are_rejected(self):
        for k, q, threads, mode in [(1, 3, 1, None), (3, 4, 1, None),
                                     (3, 5, 0, None), (3, 5, 2, 'sequential')]:
            with self.subTest(k=k, q=q, threads=threads, mode=mode):
                with self.assertRaises(ValueError):
                    common.command(Path('/data/g.bin'), k, q, threads, mode)

    def test_measurement_requires_one_valid_count_and_time(self):
        good = 'Number of 3-plex: 7\nRunning time 2.5\n'
        self.assertEqual(common.measurement(good, 3), (7, 2.5))
        for bad in [good + good, good.replace('2.5', 'nan'), good.replace('2.5', '-1'),
                    'Number of 3-plex: 7\n', 'Running time 2.5\n']:
            with self.assertRaises(ValueError):
                common.measurement(bad, 3)

    def test_cpu_selection_does_not_silently_oversubscribe(self):
        self.assertEqual(common.cpu_list(None, 4, [3, 4, 5, 6]), [3, 4, 5, 6])
        for text, width, allowed in [('0,0', 2, [0, 1]), (None, 16, list(range(9))),
                                      ('0,2', 2, [0, 1])]:
            with self.assertRaises(ValueError):
                common.cpu_list(text, width, allowed)


if __name__ == '__main__':
    unittest.main()
