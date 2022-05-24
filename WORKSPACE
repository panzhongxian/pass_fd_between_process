workspace(name = "cbstream_proxy")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

all_content = """filegroup(name = "all", srcs = glob(["**"]), visibility = ["//visibility:public"])"""

http_archive(
    name = "libevent",
    build_file_content = all_content,
    sha256 = "92e6de1be9ec176428fd2367677e61ceffc2ee1cb119035037a27d346b0403bb",
    strip_prefix = "libevent-2.1.12-stable",
    urls = ["https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz"],
)

http_archive(
    name = "rules_foreign_cc",
    strip_prefix = "rules_foreign_cc-main",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/main.zip",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()
