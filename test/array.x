auto arr1 = [
    "test",
    123,
];

echo arr1;

/*
auto arr2 = (["test", 123, arr1])[1];

auto arr3 = [
    "test" = arr2,
    123 = 123,
];
*/

auto arr4 = ["test", 123, arr1];

echo arr4;

auto arr5 = ["test", 123, arr1, [], [1, 2, 3]];

echo arr5;