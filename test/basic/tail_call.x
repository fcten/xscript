function tail(n int) int {
    if (n <= 1) {
        return n;
    }
    return tail(n - 1);
}

function main() {
    echo(tail(100000));
}
