include_directories(src/jsoncpp)
add_library(jsoncpp SHARED
    json_reader.cpp
    json_value.cpp
    json_writer.cpp
)
