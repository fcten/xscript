class MyServer extends Server {
    protected function onAccept() {

    }

    protected function onReceive() {

    }

    protected function onRequest() {
        /*
        try {
            Client cli = new Client;
            string rep = cli->get("127.0.0.1", 9503);
            echo rep;
            echo "\n";
            //return rep;
        } catch (auto e) {
            echo "get failed\n";
        }
        */
        return "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Type: application/json\r\nDate: Thu, 25 Oct 2018 14:33:53 GMT\r\nContent-Length: 56\r\n\r\n{\"Status\":0,\"Message\":\"OK\",\"Time\":\"2018-10-25 22:33:53\"}";
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