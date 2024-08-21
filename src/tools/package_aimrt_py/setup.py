# Copyright (c) 2023, AgiBot Inc.
# All rights reserved.

import setuptools
from setuptools import setup

with open("VERSION", "r") as f:
    version = f.read().strip()

setup(
    version=version,
    ext_modules=[setuptools.Extension(name="dummy", sources=[])],
)
