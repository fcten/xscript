type A interface {
    func m1();
}

type B interface {
    func m2();
}

type C int;

func (c C) m1() {
    echo "m1";
}

func (c C) m2() {
    echo "m2";
}

var obj C;

func testA(obj A) {
    obj.m1();
}

func testB(obj B) {
    obj.m2();
}

func main() {
    testA(obj);
    testB(obj);
}