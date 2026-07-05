cmake_minimum_required(VERSION 3.20)
project(metricd
    VERSION 1.0.0
    DESCRIPTION "System metrics daemon with Unix socket IPC"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
enable_testing()
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -DMETRICD_DEBUG)
endif()

add_subdirectory(src)
add_subdirectory(tests)

install(TARGETS metricd RUNTIME DESTINATION bin)
install(FILES config/metricd.default.toml
    DESTINATION share/metricd)
install(FILES packaging/systemd/metricd.service
    DESTINATION lib/systemd/user)
install(FILES packaging/tmpfiles.d/metricd.conf
    DESTINATION lib/tmpfiles.d)
install(FILES LICENSE
    DESTINATION share/licenses/${PROJECT_NAME})
install(FILES docs/ARCHITECTURE.md docs/PROTOCOL.md
    DESTINATION share/doc/${PROJECT_NAME})
