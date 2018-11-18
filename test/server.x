class MyServer extends Server {
    protected function onAccept() {

    }

    protected function onReceive() {

    }

    protected function onRequest() {
        //try {
        //    Client cli = new Client;
        //    return cli->get("58.217.200.15", 80);
        //} catch (auto e) {
            return "get failed";
        //}
    }

    protected function onClose() {

    }

    public function run() {
        this->listen(8888);
        this->start(0);
    }
}

MyServer app = new MyServer;
app->run();