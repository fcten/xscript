function test() {
    echo "test";
    return true;
}

auto a = true || test();

echo a;