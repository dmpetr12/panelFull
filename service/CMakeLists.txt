add_executable(panel-backend
    main.cpp




)

target_link_libraries(panel-backend PRIVATE
    Qt6::Core
    core
)

add_custom_command(TARGET panel-backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/web/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/web/dist"
            "${CMAKE_BINARY_DIR}/web/dist"
)
