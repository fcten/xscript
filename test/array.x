auto arr1 = [
    "test",
    123,
];

auto arr2 = (["test", 123, arr1])[1];

auto arr3 = [
    "test" = arr2,
    123 = 123,
];