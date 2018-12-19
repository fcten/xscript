Socket socket = new Socket;
socket->connect("127.0.0.1", 8888);

socket->send("hello");
echo socket->recv(1024);

socket->close();
