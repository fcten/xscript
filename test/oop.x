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
class D {
    public function print() {
        echo "hello";
    }
}

object test = new D();
test->print();