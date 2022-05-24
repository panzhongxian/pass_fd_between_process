ps -ef | grep "bazel-bin/server" | grep -v grep | awk '{print $2}' | xargs kill 
rm test_server_port_01_100* 1000.log 1001.log -rf
bazel build -c dbg //:all
stdbuf -oL bazel-bin/server 1000 &> 1000.log &
stdbuf -oL bazel-bin/server 1001 &> 1001.log & 
