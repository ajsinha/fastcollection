"""
FastCollection - Ultra High-Performance Memory-Mapped Collections with TTL

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
Patent Pending
"""

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys
import os

class get_pybind_include:
    def __str__(self):
        import pybind11
        return pybind11.get_include()

ext_modules = [
    Extension(
        'fastcollection',
        sources=[
            'src/main/python/pyfastcollection.cpp',
            'src/main/cpp/src/fc_common.cpp',
            'src/main/cpp/src/fc_list.cpp',
            'src/main/cpp/src/fc_set.cpp',
            'src/main/cpp/src/fc_map.cpp',
            'src/main/cpp/src/fc_queue.cpp',
            'src/main/cpp/src/fc_stack.cpp',
        ],
        include_dirs=[
            get_pybind_include(),
            'src/main/cpp/include',
        ],
        libraries=['boost_system', 'pthread', 'rt'],
        language='c++',
        extra_compile_args=['-std=c++20', '-O3', '-fPIC'],
    ),
]

setup(
    name='fastcollection',
    version='1.0.0',
    author='Ashutosh Sinha',
    author_email='ajsinha@gmail.com',
    description='Ultra High-Performance Memory-Mapped Collections with TTL',
    long_description=open('docs/README.md').read() if os.path.exists('docs/README.md') else '',
    long_description_content_type='text/markdown',
    ext_modules=ext_modules,
    install_requires=['pybind11>=2.6'],
    setup_requires=['pybind11>=2.6'],
    python_requires='>=3.8',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: Other/Proprietary License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: C++',
        'Topic :: Software Development :: Libraries',
    ],
)
