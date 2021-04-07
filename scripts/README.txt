TODO: convert this readme into markdown

build.sh --> 		sets environment variables (needed by our cross compiler) before calling make

build.binutils.sh --> 	assumes that the source code for binutils is in your home folder.
			builds binutils which is needed by our cross compiler (gcc)

build_gcc.sh -->	assumes that the source code for gcc is in your home folder.
			builds gcc that targets i686-elf, which will be compatible
			with StinkOS.

install_gcc_deps.sh-->	simple script that installs gcc dependencies.

