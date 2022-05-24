load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake", "configure_make")

cc_binary(
    name = "server",
    srcs = [
        "server.cc",
    ],
    deps = [
        ":libevent",
        "//apue_send_recv_fd",
    ],
)

cc_binary(
    name = "unary_client",
    srcs = [
        "unary_client.cc",
    ],
    deps = [
        ":libevent",
    ],
)

cc_binary(
    name = "stream_client",
    srcs = [
        "stream_client.cc",
    ],
    deps = [
        ":libevent",
    ],
)

# https://github.com/libevent/libevent/blob/master/Documentation/Building.md#cmake-variables
cmake(
    name = "libevent",
    cache_entries = {
        "-DEVENT__DISABLE_MBEDTLS": "ON",
        "EVENT__LIBRARY_TYPE": "STATIC",
    },
    env = select(
        {
            "@platforms//os:osx": {"PKG_CONFIG_PATH": "/usr/local/opt/openssl/lib/pkgconfig"},
            "//conditions:default": {},
        },
    ),
    lib_source = "@libevent//:all",
    out_static_libs = [
        "libevent_pthreads.a",
        "libevent_extra.a",
        "libevent.a",
        "libevent_openssl.a",
        "libevent_core.a",
    ],
)
