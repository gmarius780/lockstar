from glob import glob
from setuptools import  setup, find_packages
from pybind11.setup_helpers import Pybind11Extension
from os.path import join, dirname
import site

# Make cpp available in python via Extensions: https://docs.python.org/3/distutils/setupscript.html
ext_modules = [
    Pybind11Extension(
        'hardware',
        [join(dirname(__file__), 'lockstar_general','hardware', 'hardware_pybind.cpp')],
        cxx_std='17',
        # give the compiler access to the mc code
        extra_compile_args=[
            '-O3', 
            f"-I{join(dirname(__file__), '..', 'mc', 'MCLock', 'Core')}",
            *[f"-I{join(s, 'pybind11', 'include')}" for s in site.getsitepackages()] # TO MAKE THIS WORK: pip install pybind11 in your environment
        ]
    ),
    Pybind11Extension(
        'mc_modules',
        [join(dirname(__file__), 'lockstar_general', 'mc_modules', 'mc_modules_pybind.cpp')],
        cxx_std='17',
        # give the compiler access to the mc code
        extra_compile_args=[
            '-O3', 
            f"-I{join(dirname(__file__), '..', 'mc', 'MCLock', 'Core')}",
            *[f"-I{join(s, 'pybind11', 'include')}" for s in site.getsitepackages()] # TO MAKE THIS WORK: pip install pybind11 in your environment
        ]
    )
]
setup(
        name = "lockstar_general",
        packages=find_packages(),
        version = "1.0",
        author = "Marius Gächter",
        author_email = "gmarius@ethz.ch",
        description = "Python Code used in client as well as on rpi",
        license = "BSD",
        install_requires = ["numpy", "scipy"],
        dependency_links = [
            "http://code.enthought.com/enstaller/eggs/source",
            ],
        ext_package='lockstar_general',
        ext_modules=ext_modules,
        include_package_data = True,
        zip_safe = False,
        )
