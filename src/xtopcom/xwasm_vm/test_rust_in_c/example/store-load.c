int a;
extern void print(int a);
void set(int input) {
    a = input;
}
void add() {
    a = a + 1;
}
int show() {
    print(a);
    return a;
}