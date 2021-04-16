struct bucket {
    int a;
    float f;
};
bucket g;
extern "C" void set(int _a, float _f) {
    g.a = _a;
    g.f = _f;
    return;
}
extern "C" int geta() {
    return g.a;
}
extern "C" float getf() {
    return g.f;
}