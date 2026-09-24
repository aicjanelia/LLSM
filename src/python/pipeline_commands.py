"""Build shell jobs whose dependent modules stop on the first failure."""

import shlex


def build_job_command(submission_command, steps):
    # Module commands currently end in ';'. Join their bodies with && so a
    # later successful MIP cannot mask a failed decon exit code.
    job = ' && '.join(step.rstrip().rstrip(';') for step in steps)
    return submission_command + ' ' + shlex.quote(job)
