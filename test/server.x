class MyServer extends Server {
    protected func onAccept() {

    }

    protected func onReceive() {

    }

    protected func onRequest(string req) {
        //return req;
        try {
            Client cli = new Client;
            return cli->get("58.217.200.1", 80);
        } catch (auto e) {
            return e;
        }
    }

    protected func onClose() {

    }

    public func run() {
        this->listen(8888);
        this->start(0);
    }
}

MyServer app = new MyServer;
app->run();