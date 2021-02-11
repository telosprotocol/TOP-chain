set -e

rm -rf log3
mkdir -p log3

# foreground
./xkad_bench -a 127.0.0.1 -l 8822 -L log3/8822.log -i 1 -p 127.0.0.1:8833
