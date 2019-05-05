
var arr1 = [
    "test",
    123,
];

echo arr1;

var arr2 = (["test", 123, arr1])[2][0];

echo arr2;

var arr4 = ["test", 123, arr1];

echo arr4

var arr5 = ["test", 123, arr1, [], [1, 2, 3]]

echo arr5[4][0]

var arr6 = []

arr6[] = 3

arr6[] = [4,5,6]

arr6[4] = arr4

arr6[3] = arr2

arr6[1][] = 7

echo arr6

var arr7 = []

arr7["k-a"] = "v-a"
arr7["k-b"] = "v-b"
arr7[] = "v-c"

echo arr7

var arr8 = [
    "#": arr1,
    123: arr2,
    arr2: arr7["k-a"],
]

echo arr8

var arr9 = [
    "name": "Colin",
    "age": 20,
    "sex": "male",
    "verified": true,
]

echo arr9