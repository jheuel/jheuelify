set(TARGET_NAME jheuelify)
set(SRC
    main.cpp
    SignalHandler.cpp)

add_executable(${TARGET_NAME} ${SRC})

target_link_libraries(${TARGET_NAME} ${LIBRARIES})

#vim : ft = cmake
