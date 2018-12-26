/*
function print() {
    echo "hello world";
}

print();

int a = 1, b = 2;

function add (int c, int d) {
    auto e = 3, f = 4;
    return c+d;
}

echo add(a, b + 1);*/

function fib(int n) {
    if (n < 2) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}

echo fib(20);
/*
function test(int n) {
    return n;
}

int i = 1;
int count = 0;
while (i <= 1000000) {
    count = count + test(i);
    i = i + 1;
}
echo count;*/