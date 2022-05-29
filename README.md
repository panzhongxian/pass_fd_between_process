## 进程间传递网络连接(文件描述符)

模拟两个 Server 之间传输网络连接。做个实验的主要背景：

- 进程间传递网络连接可以被用于热重启。之前的文章 《[Envoy网关热重启原理分析](https://km.woa.com/group/34294/articles/show/437024)》中介绍过，但 Envoy 代码较重，比较难以理解相关概念。
- 与客户端有长连接建立的服务不适合滚动更新，可能不适合滚动更新
- 了解 UDS(Unix Domain Socket) 的使用方法。 APUE 第 17 章有介绍 UDS 及通过 UDS 传递文件描述符的具体做法。

项目使用 Bazel 构建，依赖 libevent 事件引擎库，也用到 APUE 示例代码（以库的方式使用）。

详细说明请见文档《[进程间传递网络连接(文件描述符)](https://panzhongxian.cn/cn/2022/05/pass-tcp-fd-between-processes/)》
