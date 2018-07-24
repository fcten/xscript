
auto arr1 = [
    "test",
    123,
];

echo arr1;

auto arr2 = (["test", 123, arr1])[2][0];

echo arr2;

auto arr4 = ["test", 123, arr1];

echo arr4;

auto arr5 = ["test", 123, arr1, [], [1, 2, 3]];

echo arr5[4][0];

auto arr6 = [];

arr6[] = 3;

arr6[] = [4,5,6];

arr6[4] = arr4;

arr6[3] = arr2;

arr6[1][] = 7;

echo arr6;