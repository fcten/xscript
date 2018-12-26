/* EXPECT
3
*/

int a = 1;
int b, c;

do {
    int a = 2;
    b = a;
} while (false);

c = a;

echo b + c;

