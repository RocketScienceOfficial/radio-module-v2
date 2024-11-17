add_library(communication_utils cobs.cpp)

target_include_directories(communication_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})