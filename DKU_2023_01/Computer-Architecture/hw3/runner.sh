mkdir results
rm results/*.out

echo "===== Testing Professor Input 1 ====="
./mipsim -i ./inputs/prof_input1/input.bin -o results/1-nf-sp.out > /dev/null
./mipsim -i ./inputs/prof_input1/input.bin -1 -o results/1-nf-1p.out > /dev/null
./mipsim -i ./inputs/prof_input1/input.bin -2 -o results/1-nf-2p.out > /dev/null
./mipsim -i ./inputs/prof_input1/input.bin -f -o results/1-f-sp.out > /dev/null
./mipsim -i ./inputs/prof_input1/input.bin -f -1 -o results/1-f-1p.out > /dev/null
./mipsim -i ./inputs/prof_input1/input.bin -f -2 -o results/1-f-2p.out > /dev/null
echo " - All files generated: DONE!"

if diff -q results/1-nf-sp.out results/1-nf-1p.out && \
   diff -q results/1-nf-sp.out results/1-nf-2p.out && \
   diff -q results/1-nf-sp.out results/1-f-sp.out && \
   diff -q results/1-nf-sp.out results/1-f-1p.out && \
   diff -q results/1-nf-sp.out results/1-f-2p.out; then
    echo -e "\e[32m - Testing PASSED!\e[0m"  # Print in green
else
    echo -e "\e[31m - Testing FAILED!\e[0m"  # Print in red
    exit 1
fi


echo "===== Testing Professor Input 2 ====="
./mipsim -i ./inputs/prof_input2/input.bin -o results/2-nf-sp.out > /dev/null
./mipsim -i ./inputs/prof_input2/input.bin -1 -o results/2-nf-1p.out > /dev/null
./mipsim -i ./inputs/prof_input2/input.bin -2 -o results/2-nf-2p.out > /dev/null
./mipsim -i ./inputs/prof_input2/input.bin -f -o results/2-f-sp.out > /dev/null
./mipsim -i ./inputs/prof_input2/input.bin -f -1 -o results/2-f-1p.out > /dev/null
./mipsim -i ./inputs/prof_input2/input.bin -f -2 -o results/2-f-2p.out > /dev/null
echo " - All files generated: DONE!"

if diff -q results/2-nf-sp.out results/2-nf-1p.out && \
   diff -q results/2-nf-sp.out results/2-nf-2p.out && \
   diff -q results/2-nf-sp.out results/2-f-sp.out && \
   diff -q results/2-nf-sp.out results/2-f-1p.out && \
   diff -q results/2-nf-sp.out results/2-f-2p.out; then
    echo -e "\e[32m - Testing PASSED!\e[0m"  # Print in green
else
    echo -e "\e[31m - Testing FAILED!\e[0m"  # Print in red
    exit 1
fi

echo "===== Testing Factorial ====="
./mipsim -i ./inputs/new_factorial/factorial.bin -o results/3-nf-sp.out > /dev/null
./mipsim -i ./inputs/new_factorial/factorial.bin -1 -o results/3-nf-1p.out > /dev/null
./mipsim -i ./inputs/new_factorial/factorial.bin -2 -o results/3-nf-2p.out > /dev/null
./mipsim -i ./inputs/new_factorial/factorial.bin -f -o results/3-f-sp.out > /dev/null
./mipsim -i ./inputs/new_factorial/factorial.bin -f -1 -o results/3-f-1p.out > /dev/null
./mipsim -i ./inputs/new_factorial/factorial.bin -f -2 -o results/3-f-2p.out > /dev/null
echo " - All files generated: DONE!"

if diff -q results/3-nf-sp.out results/3-nf-1p.out && \
   diff -q results/3-nf-sp.out results/3-nf-2p.out && \
   diff -q results/3-nf-sp.out results/3-f-sp.out && \
   diff -q results/3-nf-sp.out results/3-f-1p.out && \
   diff -q results/3-nf-sp.out results/3-f-2p.out; then
    echo -e "\e[32m - Testing PASSED!\e[0m"  # Print in green
else
    echo -e "\e[31m - Testing FAILED!\e[0m"  # Print in red
    exit 1
fi

echo "===== Testing Fibonacci ====="
./mipsim -i ./inputs/new_fibonacci/fibonacci.bin -o results/4-nf-sp.out > /dev/null
./mipsim -i ./inputs/new_fibonacci/fibonacci.bin -1 -o results/4-nf-1p.out > /dev/null
./mipsim -i ./inputs/new_fibonacci/fibonacci.bin -2 -o results/4-nf-2p.out > /dev/null
./mipsim -i ./inputs/new_fibonacci/fibonacci.bin -f -o results/4-f-sp.out > /dev/null
./mipsim -i ./inputs/new_fibonacci/fibonacci.bin -f -1 -o results/4-f-1p.out > /dev/null
./mipsim -i ./inputs/new_fibonacci/fibonacci.bin -f -2 -o results/4-f-2p.out > /dev/null
echo " - All files generated: DONE!"

if diff -q results/4-nf-sp.out results/4-nf-1p.out && \
   diff -q results/4-nf-sp.out results/4-nf-2p.out && \
   diff -q results/4-nf-sp.out results/4-f-sp.out && \
   diff -q results/4-nf-sp.out results/4-f-1p.out && \
   diff -q results/4-nf-sp.out results/4-f-2p.out; then
    echo -e "\e[32m - Testing PASSED!\e[0m"  # Print in green
else
    echo -e "\e[31m - Testing FAILED!\e[0m"  # Print in red
    exit 1
fi

exit 0
