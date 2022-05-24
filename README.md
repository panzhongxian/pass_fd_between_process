## 进程间传输文件描述符

通过实验模拟两个server之间进行 fd 的传输，依赖 libevent 和 apue 示例代码。

### server

server 的具体逻辑：

- 创建 socket 监听 TCP 端口
- accept 建立客户端连接
- 从连接上读一个请求（可能一次性发多个逻辑请求）
- 在该连接上发送返回，标识是哪个server
- 将 fd 传递给另外一个server进行后续处理

调用 `sh start_server.sh` 会构建 server 程序，并拉起两个进程，分别监听 1000 和 1001 端口，同时会创建并监听对应端口的一个命名 UDS：

![](./images/server_listen_port.png)

接收协议类型，为了能够恰好去切包。针对不能确切的切包的协议，后边会有讨论。

```c++
struct{
char len;
char data[len];
};
```

返回内容为纯文本：

```c++
rsp from port: 1000
```

每个server都会写对应端口的log文件，1000.log 和 1001.log 会接收到相同条数到内容。

两个 server 其实是完全相同的逻辑，只是监听的端口不同。这里为了简便，所以让他们监听了不同的端口，其实 Linux 允许多个进程监听同一个端口——真是场景中才有意义。

![](./images/uds_send_recv_fd.svg)

图一中也可以看出，server1建立的 fd 被传送到了 server2。



### 一元 Client

调用 `sh start_unary_client.sh` 会构建一元RPC请求的client，即发送一条请求，等待一条返回，再继续发送下一条请求，再等待返回。同时启动 100 个client，即建立100连接，每个client重复发送请求并接收返回1000次。校验前后两次返回来自于不同的server。

<img src="./images/client_print_server_port.png" alt="image-20220524122704451" style="zoom:50%;" />

### 流 Client

调用 `sh start_stream_client.sh` 会构建流RPC请求的client，即一次性发送1000条请求，然后等待返回。也是同时启动100个client并发处理。校验每个返回总条数为1000，每次返回来源于两个不同的server，且严格交替。

### 其他思考

1. 两个 server 其实可以监听相同的tcp端口
2. 无法恰好切好的协议，可以在传递fd的同时，将已经读出来的内容同时通过UDS发送给另外的 server



