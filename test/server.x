class MyServer extends Server {
    protected function onAccept() {

    }

    protected function onReceive() {

    }

    protected function onRequest(string req) {
        //return req;
        try {
            Client cli = new Client;
            return cli->get("58.217.200.1", 80);
        } catch (auto e) {
            return e;
        }
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