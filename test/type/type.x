package main;

func main() {
    type user struct{
        id int;
        name string;
    }

    var j user;
    j.id = 1;
    j.name = "name";

    echo(j);
}