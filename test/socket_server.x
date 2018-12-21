Socket socket = new Socket;
socket->bind("0.0.0.0", 8888);
socket->listen();

while (true) {
    handleConnection(socket->accept());
}

async function handleConnection (Socket conn) {
    try {
        while (true) {
            string data = conn->recv(1024);
            echo data;
            conn->send(data);
            //conn->send("HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n");
        }
    } catch (auto e) {
        echo e;
        conn->close();
    }
}