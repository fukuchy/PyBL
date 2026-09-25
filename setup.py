import os

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from Cython.Build import cythonize


# PYBL_PORTABLE=1 を指定すると, ビルドしたマシン固有の命令セットを使わない (配布用)
PORTABLE = os.environ.get("PYBL_PORTABLE", "0") == "1"


class pybl_build_ext(build_ext):
    def build_extensions(self):
        if self.compiler.compiler_type == "unix":
            args = ["-std=c++20", "-O2"]
            if not PORTABLE:
                args.append("-march=native")
        elif self.compiler.compiler_type == "msvc":
            args = ["/std:c++20", "/O2", "/utf-8"]
            if not PORTABLE:
                args.append("/arch:AVX2")
        else:
            args = []

        for e in self.extensions:
            e.extra_compile_args = args

        build_ext.build_extensions(self)


ext = Extension("pybl.pybl",
                sources=["pybl/pybl.pyx", "pybl/cpp/formation.cpp", "pybl/cpp/judge.cpp",
                         "pybl/cpp/game_state.cpp"],
                include_dirs=["pybl/cpp", "pybl/cpp/utils"],
                language="c++")

setup(
    name="pybl",
    ext_modules=cythonize([ext]),
    cmdclass={"build_ext": pybl_build_ext},
    packages=["pybl"],
    package_data={"pybl": ["py.typed", "*.pyi"]}
)
