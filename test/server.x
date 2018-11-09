class MyServer extends Server {
    protected function onRequest () {
        string rep = this->get();
        echo rep;
        echo "\n";
        return rep;
        return "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Type: application/json\r\nDate: Thu, 25 Oct 2018 14:33:53 GMT\r\nContent-Length: 56\r\n\r\n{\"Status\":0,\"Message\":\"OK\",\"Time\":\"2018-10-25 22:33:53\"}";
    }

    public function run() {
        this->listen(8888);
        this->start(0);
    }
}

MyServer app = new MyServer;
app->run();