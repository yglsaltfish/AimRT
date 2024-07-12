from setuptools import find_packages, setup

setup(
    packages=find_packages(where="."),
    package_dir={"": "."},
    package_data={"aimrt_py": ["libaimrt_*_plugin.so", "aimrt_py.*.so"]},
)
