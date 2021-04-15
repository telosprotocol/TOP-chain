mkdir build
gcc -std=c++11 -O3 main.cpp -o build/call ../target/debug/librustvm.a -lstdc++ -lpthread -ldl -lm &&
./build/call 