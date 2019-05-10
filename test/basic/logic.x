function test1() {
    print("test false");
    return false;
}

function test2() {
    print("test true");
    return true;
}

function main() {
    // true
    print(true || test2() || test1());

    // test true
    // true
    print(false || test2() || test1());

    // test false
    // test true
    // true
    print(test1() || test2() || test1());

    // test false
    // test false
    // test false
    // false
    print(test1() || test1() || test1());

    // test true
    // test false
    // false
    print(true && test2() && test1());

    // test false
    // false
    print(test1() && test2() && test1());

    // test true
    // test true
    // test false
    // false
    print(test2() && test2() && test1());
}