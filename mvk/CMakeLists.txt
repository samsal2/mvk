
target_sources(${PROJECT_NAME} PRIVATE context.hpp 
                                       context.cpp
                                       shader_types.hpp
                                       main.cpp)

add_subdirectory(types)
add_subdirectory(utility)
add_subdirectory(detail)
add_subdirectory(validation)
add_subdirectory(wrapper)
