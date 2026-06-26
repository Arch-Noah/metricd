add_library(metricd_core STATIC
    core/Daemon.cpp
    core/Config.cpp
    ../Logger/Logger.cpp
    collectors/CpuCollector.cpp
    collectors/MemoryCollector.cpp
    collectors/DiskCollector.cpp
    collectors/NetworkCollector.cpp
        ipc/Server.cpp
    ipc/ClientSession.cpp
    serialization/JsonSerializer.cpp
        ../include/metricd/ipc/Server.hpp
        ../include/metricd/ipc/ClientSession.hpp
)

target_include_directories(metricd_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/Logger
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(metricd
    ${CMAKE_SOURCE_DIR}/app/main.cpp
)

target_link_libraries(metricd_core
    PUBLIC
        nlohmann_json::nlohmann_json
        uring
    PRIVATE
        pthread
)

target_link_libraries(metricd PRIVATE metricd_core pthread uring)
