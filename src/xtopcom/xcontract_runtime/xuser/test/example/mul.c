int zero() {
    return 0;
}
int add1(int a) {
    return a + 1;
}
int add(int a, int b) {
    return a + b;
}
float addf(float a, float b) {
    return a + b;
}
int minus(int a, int b) {
    return a - b;
}
int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}