Socket socket = new Socket;
socket->bind("0.0.0.0", 8888);
socket->listen();

while (true) {
    handleConnection(socket->accept());
}

async func handleConnection (Socket conn) {
    try {
        while (true) {
            string data = conn->recv(1024);
            echo data;
            conn->send(data);
        }
    } catch (auto e) {
        echo e;
        conn->close();
    }
}