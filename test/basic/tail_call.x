function tail(n int) {
    if (n <= 1) {
        return n
    }
    return tail(n - 1)
}

echo tail(100000)