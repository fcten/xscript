int a, b;

try {
    a = 1;
    throw 1;
} catch (auto e) {
    echo "catch exception 1\n";
}

function test() {
    throw 3;
}

try {
    test();
} catch (auto e) {
    echo "catch exception 3\n";
}

throw 2;