add_library(web STATIC
    WebApiController.cpp
    WebApiController.h
)

target_link_libraries(web PRIVATE
    Qt6::Core
    core
)