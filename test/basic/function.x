/*
function print() {
    echo "hello world";
}

print();

auto a = 1, b = 2;

function add (auto c, auto d) {
    auto e = 3, f = 4;
    return c+d;
}

echo add(a, b + 1);*/

function fib(auto n) {
    if (n < 2) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}

echo fib(20);
/*
function test(auto n) {
    return n;
}

auto i = 1;
auto count = 0;
while (i <= 1000000) {
    count = count + test(i);
    i = i + 1;
}
echo count;*/