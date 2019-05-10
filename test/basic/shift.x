function main() {
    var a = 1 << 2;  // 4
    var b = 37 >> 2; // 9

    print(a << 2); // 16
    print(b >> 2); // 2
    print(a << b); // 2048
    print(a >> b); // 0
}