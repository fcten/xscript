interface A {
    public function m1();
}

interface B {
    public function m2();
}

class C implements A, B {
    private int i;
}

class C1 {
    public string msg = "C1:print";

    public function constructor(string msg) {
        this->msg = msg;
    }

    public function print() {
        echo this->msg;
    }

    public function t() {
        this->test();
    }

    public function test() {
        echo "test";
    }
}

class C2 {
    protected function print() {
        echo "C2:print";
    }
}

C1 obj1 = new C1("hello");
C2 obj2 = new C2();
// TODO 方法调用可以在编译时确定？
obj1->print();
obj2->print();

(new C1("world"))->print();