/* EXPECT
5050
*/

int a = 100000, b = 0;
while (a > 0)
{
    b = b + a;
    a = a - 1;
}
echo b;
