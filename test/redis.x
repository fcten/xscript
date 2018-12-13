Redis r = new Redis;

// connect 不能并发执行
r->connect("127.0.0.1");

async function worker() {
    int i=0;
    while (i<20000) {
        r->set("testkey", "testvalue");
        i=i+1;
    }
}

int i;
for (i = 0; i < 100; i = i + 1) {
    worker();
}