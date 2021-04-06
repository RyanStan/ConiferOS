PREFIX="$HOME/opt/cross"
TARGET=i686-elf
PATH="$PREFIX/bin:$PATH"

# TODO: make the binutils version variable somehow
cd $HOME/src
mkdir build-binutils
cd build-binutils
../binutils-2.36/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install
