function test1() {
    echo "test false";
    return false;
}

function test2() {
    echo "test true";
    return true;
}

// true
echo true || test2() || test1();

// test true
// true
echo false || test2() || test1();

// test false
// test true
// true
echo test1() || test2() || test1();

// test false
// test false
// test false
// false
echo test1() || test1() || test1();

// test true
// test false
// false
echo true && test2() && test1();

// test false
// false
echo test1() && test2() && test1();

// test true
// test true
// test false
// false
echo test2() && test2() && test1();
