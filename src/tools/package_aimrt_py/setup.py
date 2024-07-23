import setuptools
from setuptools import find_packages, setup

with open("VERSION", "r") as f:
    version = f.read().strip()

setup(
    packages=find_packages(where="."),
    package_dir={"aimrt_py": "aimrt_py"},
    include_package_data=True,
    version=version,
    ext_modules=[setuptools.Extension(name="dummy", sources=[])],
)
