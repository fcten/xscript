interface Ship {
    int hp;
}

interface Weapon {
    int damage;
}

class Battleship implements Ship, Weapon {

}

class Megathron extends Battleship {

}

object ship = new Megathron();