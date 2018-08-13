auto x = 1;

do {
    echo x;

    switch (x) {
        case 1:
            echo "x = 1";
            break;
        case 2: case 3:
            echo "x = 2 or 3";
            break;
        default:
            echo "x = default";
    }

    x = x + 1;
} while (x <= 4);