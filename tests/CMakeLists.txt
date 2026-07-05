function(add_metricd_test name source)
    add_executable(${name} ${source})
    target_include_directories(${name}
        PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/Logger
        PRIVATE ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(${name} PRIVATE metricd_core pthread uring)
    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

add_metricd_test(test_cpu_collector     unit/test_cpu_collector.cpp)
add_metricd_test(test_memory_collector  unit/test_memory_collector.cpp)
add_metricd_test(test_disk_collector    unit/test_disk_collector.cpp)
add_metricd_test(test_network_collector unit/test_network_collector.cpp)
add_metricd_test(test_config            unit/test_config.cpp)
add_metricd_test(test_temperature_collector unit/test_temperature_collector.cpp)
add_metricd_test(test_battery_collector     unit/test_battery_collector.cpp)
add_metricd_test(test_system_collector      unit/test_system_collector.cpp)
