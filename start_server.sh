ps -ef | grep "bazel-bin/server" | grep -v grep | awk '{print $2}' | xargs kill 
rm -rf test_server_port_01_100* 1000.log 1001.log
bazel build -c dbg //:all
if [[ "$OSTYPE" == "linux-gnu" ]]; then
  stdbuf -oL bazel-bin/server 1000 &> 1000.log &
  stdbuf -oL bazel-bin/server 1001 &> 1001.log & 
elif [[ "$OSTYPE" == "darwin"* ]]; then
  bazel-bin/server 1000 &> 1000.log &
  bazel-bin/server 1001 &> 1001.log & 
fi
