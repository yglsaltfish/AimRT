import setuptools
from setuptools import find_packages, setup

with open("VERSION", "r") as f:
    version = f.read().strip()

setup(
    packages=find_packages(where="."),
    package_dir={"": "."},
    package_data={"aimrt_py": ["libaimrt_*_plugin.so", "aimrt_py.*.so"]},
    version=version,
    ext_modules=[setuptools.Extension(name="dummy", sources=[])],
)
