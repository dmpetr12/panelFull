add_executable(panel-backend
    main.cpp




)

target_link_libraries(panel-backend PRIVATE
    Qt6::Core
    core
)
