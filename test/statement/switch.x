package main;

func main() {
    var x = 1;

    do {
        echo(x);

        switch (x) {
            case 1:
                echo("x = 1");
                break;
            case 2: case 3:
                echo("x = 2 or 3");
                break;
            default:
                echo("x = default");
        }

        x = x + 1;
    } while (x <= 4);

    var y = [
        "test case 1",
        "test case 2",
        "test case 3",
        "test case 4",
    ];

    var i int;
    for (i = 0; i < 4; i = i + 1) {
        echo(y[i]);
        switch (y[i]) {
            case "test case 1":
                echo("test case 1");
                break;
            case "test case 2":
            case "test case 3":
                echo("test case 2 or 3");
                break;
            default:
                echo("test case default");
        }
    }
}