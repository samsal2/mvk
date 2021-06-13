target_sources(${PROJECT_NAME} PRIVATE compressed_pair.hpp
                                       concepts.hpp
                                       misc.hpp
                                       slice.hpp
                                       types.hpp
                                       trace.cpp
                                       trace.hpp
                                       verify.cpp
                                       verify.hpp)
	
add_subdirectory(meta)
