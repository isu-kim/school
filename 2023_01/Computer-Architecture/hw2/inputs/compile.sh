#!/bin/bash

IN_DIR=$(readlink -f $1)
C_FILE=$(ls $IN_DIR | grep ".c")
echo Input: $IN_DIR/$C_FILE

docker run -it --rm -v $IN_DIR:/root boanlab/mips-ubuntu mips-compile /root/$C_FILE
echo "Done!, Written .o and .s"
