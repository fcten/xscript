function tail(n int) int {
    if (n <= 1) {
        return n;
    }
    return tail(n - 1);
}

function main() {
    print(tail(100000));
}
