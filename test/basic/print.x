package main;

func main() {
    echo("hello world");
    echo("你好，世界！");
    //echo(null);
    echo(123);
    echo(123.6);
    echo(true);
    echo(false);
    echo([1, 2, 3]);
    //echo({"hello": "world"});

    echo(1 > 2);

    var a = 33;
    var b = true;
    echo(a > 1);
    echo(!b);

    echo(-23);

    echo(1 + a); // 34
    echo(a + 1); // 34
    echo(1 - a); // -32
    echo(a - 1); // 32
    echo(3 * a); // 99
    echo(a * 3); // 99
    echo(3 / a); // 0
    echo(a / 3); // 11
}