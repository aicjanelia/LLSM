import sys
from pathlib import Path
import subprocess
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from pipeline_commands import build_job_command


class PipelineCommandTests(unittest.TestCase):
    def run_job(self, steps):
        # sh -c stands in for the scheduler accepting a single job string.
        return subprocess.run(build_job_command('sh -c', steps), shell=True,
                              capture_output=True, text=True)

    def test_failed_decon_preserves_exit_and_skips_mip(self):
        result = self.run_job(["printf 'deskew done\\n';", "sh -c 'exit 7';",
                               "printf 'mip must not run\\n';"])
        self.assertEqual(result.returncode, 7)
        self.assertEqual(result.stdout, 'deskew done\n')

    def test_success_runs_all_steps_in_order(self):
        result = self.run_job(["printf 'deskew\\n';", "printf 'decon\\n';",
                               "printf 'mip\\n';"])
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, 'deskew\ndecon\nmip\n')

    def test_last_step_failure_is_not_masked(self):
        self.assertEqual(self.run_job(['true;', "sh -c 'exit 9';"]).returncode, 9)


if __name__ == '__main__':
    unittest.main()
