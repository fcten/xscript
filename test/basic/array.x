
function main() {
    var arr1 []int = [];
    echo(arr1);

    var arr2 = [1];
    echo(arr2);

    var arr3 = ([[1,2,3],[4,5,6],[7,8,9]])[1][1];
    echo(arr3);

    var arr4 = [123, 456, arr2[0]];
    echo(arr4);

    var arr5 = ([[1,2,3],[4,5,6],arr2])[2][0];
    echo(arr5);

    var arr6 = [["123","456"],["789"],["321","654"]];
    echo(arr6[0][1]);

    var arr7 []int = [];
    arr7[] = 3;
    arr7[] = 4;
    arr7[] = 5;
    arr7[4] = 6;
    arr7[arr2[0]] = 7;
    echo(arr7);
    echo(arr7[3]);

    var arr8 [][]int = [];
    arr8[] = arr1;
    arr8[] = [];
    echo(arr8);
}