
target_sources(${PROJECT_NAME} PRIVATE shader_types.hpp
                                       main.cpp)

add_subdirectory(utility)
add_subdirectory(detail)
add_subdirectory(engine)
