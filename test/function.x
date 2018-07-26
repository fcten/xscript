/*
function print() {
    echo "hello world";
}

print();

auto a = 1, b = 2;

function add (auto c, auto d) {
    auto e = 3, f = 4;
    return a + b + c + d + e + f;
}

echo add(a, b + 1);

function fib(auto n, auto fib) {
    if (n < 2) {
        return n;
    }
    return fib(n-1, fib) + fib(n-2, fib);
}

echo fib(30, fib);*/

function test(auto n) {
    return n;
}

auto i = 1;
auto count = 0;
while (i <= 1000000) {
    count = count + test(i);
    i = i + 1;
}
echo count;