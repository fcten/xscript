function print() {
    echo "hello world";
}

echo "call print";
print();
echo "call end";

auto a = 1, b = 2;

function add (auto c, auto d) {
    auto e = 3, f = 4;
    return a + b + c + d + e + f;
}

echo add(a, b + 1);

function fib(auto n) {
    if (n < 2) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}

echo fib(50);