# Computer_Architecture
An implementation for HW3 from Computer Architecture spring semester.

## Building
mipsim also offers Makefile. You can build mipsim using following command.
```
make all
```
Or you can clean up your compiled files using
```
make clean
```

## Commandline Interface
- `-h`, `--help`: Print this help message and exit
- `-i`, `--input`: Specify an input file to execute
- `-o`, `--output`: Specify an output file to dump log into
- `-f`, `--detect_forward`: Detecting forward and bypass
- `-s`, `--bp_static`: For Static Branch prediction
- `-1`, `--bp_1bit`: For 1 bit Dynamic Branch prediction
- `-2`, `--bp_2bit`: For 2 bit Dynamic prediction

Example Command
```
./mipsim -i ./inputs/prof_input1/input.bin -f -2 -o out.txt
```

## LICENSE
GPL 3.0 license. Please do the right thing.