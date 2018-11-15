async function print(int from, int to) {
    int i;
    for (i = from; i <= to; i = i + 1) {
        echo i;
        echo "\n";
        co_sleep(100);
    }
}

class MyClass {
    public string name = "MyClass: ";

    async public function print(int from, int to) {
        int i;
        for (i = from; i <= to; i = i + 1) {
            echo this->name;
            echo i;
            echo "\n";
            co_sleep(100);
        }
    }
}

echo "main start\n";

print(100, 110);
print(200, 210);

MyClass obj = new MyClass;
obj->print(300, 310);
obj->print(400, 410);

/*
await co_1();
await co_2();
*/

echo "main end\n";