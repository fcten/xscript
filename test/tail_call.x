function tail(int n) {
    if (n <= 1) {
        return n;
    }
    return tail(n - 1);
}

echo tail(1000);