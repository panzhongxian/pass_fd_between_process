ps -ef | grep "bazel-bin/unary_client" | grep -v grep | awk '{print $2}' | xargs kill 
bazel build -c dbg //:all
bazel-bin/unary_client 1000 
