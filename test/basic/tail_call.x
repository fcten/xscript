func tail_call(n int) int {
    if (n <= 1) {
        return n;
    }
    return tail_call(n - 1);
}

func call(n int) int {
    if (n <= 1) {
        return n;
    }
    return call(n - 1) + 1;
}

func main() {
    echo(tail_call(100000));
    echo(call(100000));
}
