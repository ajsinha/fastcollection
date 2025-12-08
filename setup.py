"""
FastCollection - Ultra High-Performance Memory-Mapped Collections with TTL

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
Patent Pending
"""

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
import sys
import os

class get_pybind_include:
    def __str__(self):
        import pybind11
        return pybind11.get_include()

# Platform-specific settings
extra_compile_args = ['-std=c++20', '-O3', '-fPIC']
extra_link_args = []
libraries = ['pthread', 'rt']

if sys.platform == 'darwin':
    extra_compile_args = ['-std=c++20', '-O3', '-fPIC', '-mmacosx-version-min=10.14']
    libraries = ['pthread']  # No 'rt' on macOS
elif sys.platform == 'win32':
    extra_compile_args = ['/std:c++20', '/O2']
    libraries = []
    extra_link_args = []

ext_modules = [
    Extension(
        'fastcollection._native',  # Extension as submodule
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
        libraries=libraries,
        language='c++',
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
    ),
]

# Read long description
long_description = ''
if os.path.exists('docs/README.md'):
    with open('docs/README.md', 'r', encoding='utf-8') as f:
        long_description = f.read()
elif os.path.exists('README.md'):
    with open('README.md', 'r', encoding='utf-8') as f:
        long_description = f.read()

setup(
    name='fastcollection',
    version='1.0.0',
    author='Ashutosh Sinha',
    author_email='ajsinha@gmail.com',
    description='Ultra High-Performance Memory-Mapped Collections with TTL',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/abhikarta/fastcollection',
    packages=['fastcollection'],
    package_dir={'fastcollection': 'src/main/python/fastcollection'},
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
        'Programming Language :: Python :: 3.12',
        'Programming Language :: C++',
        'Topic :: Software Development :: Libraries',
        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows',
    ],
    keywords='collections, memory-mapped, ttl, cache, high-performance',
)
