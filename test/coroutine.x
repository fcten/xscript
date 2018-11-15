async function co_1() {
    int i;
    for (i = 0; i < 10; i = i + 1) {
        echo i;
        echo "\n";
        co_sleep(97);
    }
}

async function co_2() {
    int i;
    for (i = 10; i < 20; i = i + 1) {
        echo i;
        echo "\n";
        co_sleep(71);
    }
}

echo "main start\n";

co_1();
co_2();
/*
await co_1();
await co_2();
*/

echo "main end\n";