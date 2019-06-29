type A interface {
    function m1();
}

type B interface {
    function m2();
}

type C int;

function (c C) m1() {
    echo "m1";
}

function (c C) m2() {
    echo "m2";
}

var obj C;

function testA(obj A) {
    obj.m1();
}

function testB(obj B) {
    obj.m2();
}

function main() {
    testA(obj);
    testB(obj);
}