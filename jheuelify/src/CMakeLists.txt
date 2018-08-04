set(TARGET_NAME jheuelify)
set(SRC
    SignalHandler.cpp)

add_executable(${TARGET_NAME} main.cpp ${SRC})
target_link_libraries(${TARGET_NAME} ${LIBRARIES})

# Build a library from all specified source files
# This is required for using googletest
add_library(${TARGET_NAME}-lib ${SRC})
target_link_libraries(${TARGET_NAME}-lib ${LIBRARIES})
set_target_properties(${TARGET_NAME}-lib PROPERTIES OUTPUT_NAME jheuelify)

# vim:ft=cmake
