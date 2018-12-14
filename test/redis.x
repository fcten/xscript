try {
    Redis r = new Redis("127.0.0.1");

    echo r->exec(["SADD", "testset", "testvalue1", "testvalue2", "testvalue3"]);
    echo r->exec(["SMEMBERS", "testset"]);
} catch (auto e) {
    echo e;
}
