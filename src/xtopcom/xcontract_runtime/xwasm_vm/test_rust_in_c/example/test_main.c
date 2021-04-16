extern int test_import(int k);
extern int test_import2(int k, int i);

int main() {
    int a = 1, b = 2;
    test_import(a);
    test_import2(a, b);
    return a + b;
}