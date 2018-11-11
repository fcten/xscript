int a, b;

try {
    a = 1;
    throw 1;
} catch (auto e) {

} finally {
    
}

function test() {
    try {
        throw 3;
    } catch (auto e) {

    }
}

throw 2;