/* EXPECT
109
*/

auto a = 100;
auto b = 10;
if (a > 0) {
    a = a - 1;
}
if (b < 0) {
    b = b + 1;
}
echo a + b;

/* EXPECT
111
*/

a = 100;
b = 10;
if (a > 0) {
    a = a - 1;
} else {
    a = a + 1;
}
if (b < 0) {
    b = b - 2;
} else {
    b = b + 2;
}
echo a + b;

/* EXPECT
1
"a <= 1"
2
"a > 1"
3
"a > 2"
*/

a = 1;
while (a < 4) {
    echo a;
    if (a > 2) {
        echo "a > 2";
    } else if (a > 1) {
        echo "a > 1";
    } else {
        echo "a <= 1";
    }
    a = a + 1;
}