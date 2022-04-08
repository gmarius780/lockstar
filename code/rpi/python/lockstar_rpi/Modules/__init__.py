from os.path import dirname, basename, isfile, join
import glob
try:
    __import__('pkg_resources').declare_namespace(__name__)
    # modules = glob.glob(join(dirname(__file__), "*Module.py"))
    # __all__ = [ basename(f)[:-3] for f in modules if isfile(f) and not f.endswith('__init__.py')]
    # print(__all__)
except Exception as ex:
    print(ex)
    
