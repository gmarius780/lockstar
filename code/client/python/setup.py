from setuptools import setup, find_packages
from glob import glob

setup(
        name = "lockstar_client",
        version = "1.0",
        author = "Marius Gächter",
        author_email = "gmarius@ethz.ch",
        description = "Python Code running on client on the LOckstar",
        license = "BSD",
        install_requires = ["numpy", "scipy", "lockstar_general"],
        dependency_links = [
            "http://code.enthought.com/enstaller/eggs/source",
            ],
        packages = find_packages(),
        namespace_packages = ["lockstar_client"],
        include_package_data = True,
        zip_safe = False,
        )
