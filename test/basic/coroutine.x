function print(from int, to int) {
    var i int
    for (i = from; i <= to; i = i + 1) {
        echo i
        co_sleep(10)
    }
}

echo "main start"

co print(100, 110)
print(200, 210)

echo "main end"