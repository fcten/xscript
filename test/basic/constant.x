const string WELCOME = "Hello" + " " + "World";

echo WELCOME;

const int MAX_NUMBER = 100 * 100;

echo MAX_NUMBER;

function test() float {
    const float MAX_PRICE = 10.1;
    echo MAX_PRICE;
    return MAX_PRICE;
}

echo test();

class Test {
    public const string INFO = WELCOME;

    public function print() {
        echo this->INFO;
    }
}

Test t = new Test;
t->print();

echo t->INFO;