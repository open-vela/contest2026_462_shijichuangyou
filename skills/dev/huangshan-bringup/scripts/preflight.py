#!/usr/bin/env python3
"""Read-only host prerequisite report. Does not open a serial port."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess


def inspect(workspace):
    root = Path(workspace).resolve()
    if not root.is_dir():
        raise ValueError('Workspace directory does not exist')
    tools = {}
    for name, arguments in [('git', ['--version']), ('git-lfs', ['version']),
                            ('cmake', ['--version']), ('ninja', ['--version']),
                            ('arm-none-eabi-gcc', ['--version'])]:
        executable = shutil.which(name)
        item = {'available': bool(executable), 'path': executable}
        if executable:
            try:
                p = subprocess.run([executable, *arguments], capture_output=True,
                                   text=True, timeout=10)
                item.update(exit_code=p.returncode,
                            version=(p.stdout.splitlines() or [''])[0])
            except (OSError, subprocess.TimeoutExpired) as e:
                item.update(error=type(e).__name__)
        tools[name] = item
    required = ['nuttx/CMakeLists.txt', 'apps/CMakeLists.txt',
                'vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/configs/nsh/defconfig']
    serial = []
    for path in sorted(Path('/dev/serial/by-id').glob('*')):
        serial.append({'device': str(path.resolve()), 'identity': path.name,
                       'read_write_access': os.access(path, os.R_OK | os.W_OK),
                       'hardware_identity_confirmed': False})
    return {'workspace': str(root), 'repo_initialized': (root / '.repo/manifest.xml').is_file(),
            'free_bytes': shutil.disk_usage(root).free, 'tools': tools,
            'source_files': {x: (root / x).is_file() for x in required},
            'serial': serial, 'firmware_build': 'NOT_RUN', 'hardware_validation': 'NOT_RUN',
            'note': 'File presence does not certify a complete repo sync or build.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', required=True)
    args = parser.parse_args()
    print(json.dumps(inspect(args.workspace), ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
