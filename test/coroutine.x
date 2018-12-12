async function print(int from, int to) {
    int i;
    for (i = from; i <= to; i = i + 1) {
        echo i;
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
            co_sleep(100);
        }
    }
}

echo "main start";

print(100, 110);
await print(200, 210);

MyClass obj = new MyClass;

obj->print(300, 310);
await obj->print(400, 410);

echo "main end";