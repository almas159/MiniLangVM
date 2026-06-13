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

    print("names=", d.names, "\n");
    print("ids=", d.ids, "\n");
    print("scores=", d.scores, "\n");
    print("flags=", d.flags, "\n");
    print("points=", d.points, "\n");
    print("len(names)=", len(d.names), "\n");
    print("len(ids)=", len(d.ids), "\n");

    d.names = ["hello", "world"];
    d.ids = [10, 20, 30];
    d.scores = [7.5, 8.5];
    d.flags = [false, true];
    d.points = [
        Point { x: 9, y: 9 },
        Point { x: 8, y: 8 }
    ];

    print("names2=", d.names, "\n");
    print("ids2=", d.ids, "\n");
    print("scores2=", d.scores, "\n");
    print("flags2=", d.flags, "\n");
    print("points2=", d.points, "\n");

    if (len(d.names) == 2 && len(d.ids) == 3 && d.names == ["hello", "world"]) {
        return 0;
    }

    return 1;
}
