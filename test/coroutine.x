function co_1() {
    int i;
    for (i = 0; i < 10; i=i+1) {
        echo i;
        echo "\n";
        co_yield();
    }
}

function co_2() {
    int i;
    for (i = 10; i < 20; i=i+1) {
        echo i;
        echo "\n";
        co_yield();
    }
}

echo "main\n";

co_create(co_1);
co_create(co_2);