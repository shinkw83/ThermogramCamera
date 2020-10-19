#!/bin/bash

if [ -d "build" ]
then
	echo "build directory is exist."
else
	echo "create build directory"
	mkdir build
fi

cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../
make
make install
