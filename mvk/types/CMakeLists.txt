add_subdirectory(detail)
add_subdirectory(validation)

target_sources(${PROJECT_NAME} PRIVATE
                                       window.hpp 
                                       window.cpp
                                       types.hpp 
                                       common.hpp)
