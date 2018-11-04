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

    public function print() {
        this->msg = "C1:print\n";
        echo this->msg;
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
obj1->print();
obj2->print();