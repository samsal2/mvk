
target_sources(${PROJECT_NAME} PRIVATE buffer_manager.hpp 
                                       buffer_manager.cpp
                                       factories.hpp 
                                       factories.cpp
                                       renderer.hpp 
                                       renderer.cpp
                                       main.cpp)

add_subdirectory(vk_types)
add_subdirectory(utility)
