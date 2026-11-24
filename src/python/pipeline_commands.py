"""Build shell jobs whose dependent modules stop on the first failure."""

import shlex


def build_job_command(submission_command, steps):
    # Module commands currently end in ';'. Join their bodies with && so a
    # later successful MIP cannot mask a failed decon exit code.
    job = ' && '.join(step.rstrip().rstrip(';') for step in steps)
    return submission_command + ' ' + shlex.quote(job)


def parse_output_spacing(value):
    """Validate the rotate 'output-spacing' config value.

    Accepts the string 'input' (keep the input voxel spacing), a single positive number (cubic
    voxels) or a list of three positive numbers [x, y, z], all in um. Returns
    {'flag': '--output-spacing', 'arg': <cli string>, 'values': 'input' | [x, y, z]} where the
    'values' entry is None-free and always resolved to three numbers unless it is 'input'.
    Raises ValueError for anything else.
    """
    def positive(v):
        # bool is an int subclass, so reject it explicitly
        if isinstance(v, bool) or not isinstance(v, (int, float)):
            return False
        return 0.0 < v < 1.0e6

    if value == 'input':
        return {'flag': '--output-spacing', 'arg': 'input', 'values': 'input'}
    if positive(value):
        values = [float(value)] * 3
    elif isinstance(value, list) and len(value) == 3 and all(positive(v) for v in value):
        values = [float(v) for v in value]
    else:
        raise ValueError('output-spacing \'%s\' must be \'input\', a positive number (um), or a list of three positive numbers [x, y, z] (um)' % (value,))
    return {'flag': '--output-spacing', 'arg': ','.join(repr(v) for v in values), 'values': values}


def rotate_output_spacing(params_rotate, xy_in, z_in):
    """Output (x, y, z) spacing in um that rotate will write for the given input spacing."""
    spec = params_rotate.get('output-spacing')
    if spec is None:
        return (xy_in, xy_in, xy_in) # rotate default: cubic voxels at the input xy size
    if spec['values'] == 'input':
        return (xy_in, xy_in, z_in)
    return tuple(spec['values'])


def rotate_spacing_warnings(params_rotate, xy_in, z_in):
    """Warnings for a user-chosen rotate output spacing (none for the default or 'input').

    - an output axis finer than the smallest input spacing (almost always xy) only oversamples
      (no new information, and the output grows as 1/(x*y*z)); finer than the input z spacing
      alone is normal and does not warn
    - an output axis coarser than the input z spacing (the coarsest input axis) is real
      downsampling by linear interpolation without pre-filtering
    """
    spec = params_rotate.get('output-spacing')
    if spec is None or spec['values'] == 'input':
        return []
    tol = 1e-9
    warnings = []
    finest = min(xy_in, z_in)
    for axis, out in zip('xyz', spec['values']):
        if out < finest * (1.0 - tol):
            warnings.append('rotate output-spacing %s = %g um is finer than the smallest input spacing of %g um (oversampling adds no information and enlarges the output)' % (axis, out, finest))
        if out > z_in * (1.0 + tol):
            warnings.append('rotate output-spacing %s = %g um is coarser than the input z spacing of %g um (downsampling without pre-filtering can alias)' % (axis, out, z_in))
    return warnings
