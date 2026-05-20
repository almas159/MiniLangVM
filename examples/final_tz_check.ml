type Meters = int32;

struct Point {
    int32 x;
    int32 y;
}

T id<T>(T x) {
    return x;
}

uint make_uint() {
    return 7;
}

int main() {
    let immutable = 10;
    var inferred = 20;

    Meters distance = 100;

    uint a = 2;
    uint b = cast<uint>(3);
    uint c = a + b;
    uint d = c * cast<uint>(4);
    uint e = d / cast<uint>(2);
    uint f = e % cast<uint>(5);

    int32[3] arr = [1, 2, 3];
    int32[3] arr2 = [1, 2, 3];

    Point p = Point { x: 1, y: 2 };
    Point q = Point { x: 1, y: 2 };

    string s = id<string>("hello");
    int32 n = id<int32>(42);

    print("immutable=", immutable, "\n");
    print("inferred=", inferred, "\n");
    print("distance=", distance, "\n");
    print("uint c=", c, "\n");
    print("uint d=", d, "\n");
    print("uint e=", e, "\n");
    print("uint f=", f, "\n");
    print("len(arr)=", len(arr), "\n");
    print("typeid(n)=", typeid(n), "\n");
    print("typeof(s)=", typeof(s), "\n");
    print("sizeof(n)=", sizeof(n), "\n");

    if (arr == arr2 && p == q && c < d && make_uint() == cast<uint>(7)) {
        return 0;
    }

    return 1;
}
