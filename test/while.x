/* EXPECT
5050
*/

auto a = 100, b = 0
while (a > 0) {
    b = b + a
    a = a - 1
}
echo b
