
function main() {
    var arr1 = [
        "test",
        123,
    ];

    print(arr1);

    var arr2 = (["test", 123, arr1])[2][0];

    print(arr2);

    var arr4 = ["test", 123, arr1];

    print(arr4);

    var arr5 = ["test", 123, arr1, [], [1, 2, 3]];

    print(arr5[4][0]);

    var arr6 = [];

    arr6[] = 3;

    arr6[] = [4,5,6];

    arr6[4] = arr4;

    arr6[3] = arr2;

    arr6[1][] = 7;

    print(arr6);

    var arr7 = [];

    arr7["k-a"] = "v-a";
    arr7["k-b"] = "v-b";
    arr7[] = "v-c";

    print(arr7);

    var arr8 = {
        "#": arr1,
        123: arr2,
        arr2: arr7["k-a"],
    };

    print(arr8);

    var arr9 = {
        "name": "Colin",
        "age": 20,
        "sex": "male",
        "verified": true,
    };

    print(arr9);
}