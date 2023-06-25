cmake_minimum_required(VERSION 3.23)
project(mbedtls_bitmex_client)

set(CMAKE_CXX_STANDARD 14)

add_executable(ssl_client main.cpp WSClient.cpp BitmexClient.cpp WSUtils.cpp BitmexClient.h WSClient.h WSUtils.h)

target_link_libraries(ssl_client -lmbedtls)
target_link_libraries(ssl_client -lmbedx509)
target_link_libraries(ssl_client -lmbedcrypto)