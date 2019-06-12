function test(from int, to int) {
    var i int;
    for (i = from; i <= to; i = i + 1) {
        echo(i);
        //co_sleep(10);
    }
}

function main() {
    echo("main start");

    co test(100, 110);
    test(200, 210);

    echo("main end");
}