function fib(n int) {
    if (n < 2) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}

function main() {
    print(fib(20));
}