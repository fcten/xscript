function main () {
    /* EXPECT
    109
    */

    var a = 100;
    var b = 10;
    if (a > 0) {
        a = a - 1;
    }
    if (b < 0) {
        b = b + 1;
    }
    print(a + b);

    /* EXPECT
    111
    */

    var c = 100;
    var d = 10;
    if (c > 0) {
        c = c - 1;
    } else {
        c = c + 1;
    }
    if (d < 0) {
        d = d - 2;
    } else {
        d = d + 2;
    }
    print(c + d);

    /* EXPECT
    1
    "e <= 1"
    2
    "e > 1"
    3
    "e > 2"
    */

    var e = 1;
    while (e < 4) {
        print(e);
        if (e > 2) {
            print("e > 2");
        } else if (e > 1) {
            print("e > 1");
        } else {
            print("e <= 1");
        }
        e = e + 1;
    }
}
