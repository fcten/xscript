function factorial(int n, int acc) {
    if (n <= 1) {
        return acc;
    }
    return factorial(n - 1, n * acc);
}

echo factorial(10, 1);