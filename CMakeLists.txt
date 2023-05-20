project(wyagi)
cmake_minimum_required(VERSION 3.20)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.74.0 COMPONENTS program_options) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    add_subdirectory(fmt)
    set(CMAKE_CXX_FLAGS "--std=c++2a")

    add_executable(wyagit main.cpp GitRepository.cpp) 
    target_link_libraries(wyagit ${Boost_LIBRARIES} fmt)
endif()
