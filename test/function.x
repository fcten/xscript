/*
function print() {
    echo "hello world";
}

print();
*/
auto a = 1, b = 2;

function add (auto c, auto d) {
    //auto e = 3, f = 4;
    //return a + b + c + d + e + f;
    return c + d + 1;
}

echo add(a, b + 1);
/*
function fib(auto n) {
    if (n < 2) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}

echo fib(50);*/