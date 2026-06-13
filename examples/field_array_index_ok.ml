struct Point {
    int32 x;
    int32 y;
}

struct Data {
    string[2] names;
    uint[3] ids;
    float64[2] scores;
    bool[2] flags;
    Point[2] points;
}

int main() {
    Data d = Data {
        names: ["aa", "bb"],
        ids: [1, 2, 3],
        scores: [1.5, 2.5],
        flags: [true, false],
        points: [
            Point { x: 1, y: 2 },
            Point { x: 3, y: 4 }
        ]
    };

    print("name0=", d.names[0], "\n");
    print("name1=", d.names[1], "\n");
    print("id2=", d.ids[2], "\n");
    print("score0=", d.scores[0], "\n");
    print("flag1=", d.flags[1], "\n");
    print("point0=", d.points[0], "\n");

    d.names[0] = "hello";
    d.ids[1] = 20;
    d.scores[1] = 7.5;
    d.flags[1] = true;
    d.points[0] = Point { x: 9, y: 9 };

    print("name0b=", d.names[0], "\n");
    print("id1b=", d.ids[1], "\n");
    print("score1b=", d.scores[1], "\n");
    print("flag1b=", d.flags[1], "\n");
    print("point0b=", d.points[0], "\n");

    if (d.names[0] == "hello" &&
        d.ids[1] == cast<uint>(20) &&
        d.scores[1] == 7.5 &&
        d.flags[1] == true &&
        d.points[0] == Point { x: 9, y: 9 }) {
        return 0;
    }

    return 1;
}
