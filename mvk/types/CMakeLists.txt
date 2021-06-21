add_subdirectory(detail)

target_sources(${PROJECT_NAME} PRIVATE window.hpp 
                                       window.cpp
                                       types.hpp 
                                       common.hpp)
