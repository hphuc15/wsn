add_library(sx127x STATIC sx127x.c)
target_include_directories(sx127x PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})