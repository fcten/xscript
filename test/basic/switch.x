function main() {
    var x = 1;

    do {
        print(x);

        switch (x) {
            case 1:
                print("x = 1");
                break;
            case 2: case 3:
                print("x = 2 or 3");
                break;
            default:
                print("x = default");
        }

        x = x + 1;
    } while (x <= 4);

    array y = [
        "test case 1",
        "test case 2",
        "test case 3",
        "test case 4",
    ];

    var i int;
    for (i = 0; i < 4; i = i + 1) {
        print(y[i]);
        switch (y[i]) {
            case "test case 1":
                print("test case 1");
                break;
            case "test case 2":
            case "test case 3":
                print("test case 2 or 3");
                break;
            default:
                print("test case default");
        }
    }
}