function main() {
    var a int;
    var b float;
    var c bool, d string;
    var e []int;
    var f [int]string;
    var g struct {
        var id int;
    };
    var h interface{};
    var i function();

    type user struct {
        var id int;
        var name string;
    }

    var j user;
}