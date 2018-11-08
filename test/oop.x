/*
interface A {
    public function m1();
}

interface B {
    public function m2();
}

class C implements A, B {
    private int i;
}
*/
class C1 {
    public string msg;

    public function print(string msg) {
        this->msg = msg;
        echo this->msg;
    }

    public function t() {
        this->test();
    }

    public function test() {
        echo "test\n";
    }
}

class C2 {
    public function print() {
        echo "C2:print\n";
    }
}

C1 obj1 = new C1();
C2 obj2 = new C2();
// TODO 方法调用可以在编译时确定
obj1->t();
obj1->print("C1:print\n");
obj2->print();