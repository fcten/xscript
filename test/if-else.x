/* EXPECT
111
*/

auto a = 100, b = 10;
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
