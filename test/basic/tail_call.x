function tail_call(n int) int {
    if (n <= 1) {
        return n;
    }
    return tail_call(n - 1);
}

function call(n int) int {
    if (n <= 1) {
        return n;
    }
    return call(n - 1) + 1;
}

function main() {
    echo(tail_call(100000));
    echo(call(100000));
}
