try {
    throw 1;
} catch (auto e) {
    echo "catch exception 1\n";
}

function test() {
    throw 2;
}

try {
    test();
} catch (auto e) {
    echo "catch exception 2\n";
}

throw 3;