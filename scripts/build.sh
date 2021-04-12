#/bin/bash

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

DIR=$(dirname $(readlink -f $0))

cd $DIR/..
make 
