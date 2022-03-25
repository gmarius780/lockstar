from setuptools import setup, find_packages
from glob import glob

setup(
        name = "lockstar_general",
        version = "1.0",
        author = "Marius Gächter",
        author_email = "gmarius@ethz.ch",
        description = "Python Code used in client as well as on rpi",
        license = "BSD",
        install_requires = ["numpy", "scipy"],
        dependency_links = [
            "http://code.enthought.com/enstaller/eggs/source",
            ],
        packages = find_packages(),
        namespace_packages = ["lockstar_general"],
        include_package_data = True,
        zip_safe = False,
        )
