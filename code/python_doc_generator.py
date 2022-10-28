#!/usr/bin/env python

""" Generates static html files which contain the code documentation of the python code of the lockstar project.
The generated files are stored in doc/python_doc"""

from pdoc import pdoc
from os.path import join, dirname, isfile
from pathlib import Path
import subprocess


def find_modules(code_dir):
    modules = []
    #list of directories which contain python code to be documented
    base_dirs = [join(code_dir, 'client', 'python'),
                join(code_dir, 'general'),
                join(code_dir, 'rpi', 'python')
                ]

    for base_dir in base_dirs:
        python_files = Path(join(base_dir)).rglob('*.py')

        modules.extend([str(f) for f in python_files if isfile(f) and not str(f).endswith('__init__.py') and not str(f).endswith('setup.py') and not 'temp_old' in str(f)])

    return modules

if __name__ == "__main__":
    output_dir = join(dirname(__file__), '..','doc','python_doc')
    code_dir = dirname(__file__)

    pdoc_call = ['pdoc', '-o', output_dir]
    pdoc_call.extend(find_modules(code_dir))
    subprocess.run(pdoc_call)

    print('Now the documentation in doc/python_doc should be updated. If it did not work, make sure pdoc is installed on your machine (pip install pdoc)')