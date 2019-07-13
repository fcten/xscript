func main() {
    var a = 1 << 2;  // 4
    var b = 37 >> 2; // 9
    var c = ~1;

    echo(a << 2); // 16
    echo(b >> 2); // 2
    echo(a << b); // 2048
    echo(a >> b); // 0

    echo(~a); // -5
    echo(~b); // -10
    echo(c);  // -2
}