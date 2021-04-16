extern int fib(int n);
extern void print(int n);
int test_ext(int n) {
    print(n);
    return fib(n);
}
int zero() {
    return 0;
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
int self_fib(int n) {
    if (n <= 1)
        return n;
    return self_fib(n - 1) + self_fib(n - 2);
}