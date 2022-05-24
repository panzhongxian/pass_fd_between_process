ps -ef | grep "bazel-bin/stream_client" | grep -v grep | awk '{print $2}' | xargs kill 
bazel build -c dbg //:all
bazel-bin/stream_client 1000
