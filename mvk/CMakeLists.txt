
target_sources(${PROJECT_NAME} PRIVATE buffer_manager.hpp 
                                       buffer_manager.cpp
                                       renderer.hpp 
                                       renderer.cpp
                                       basic_types.hpp
                                       shader_types.hpp
                                       main.cpp)

add_subdirectory(types)
add_subdirectory(utility)
add_subdirectory(detail)
add_subdirectory(validation)
add_subdirectory(wrapper)
