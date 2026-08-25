add_library(asio INTERFACE)

target_include_directories(asio INTERFACE ${CMAKE_SOURCE_DIR}/external/asio/include)

target_compile_definitions(asio INTERFACE ASIO_STANDALONE)