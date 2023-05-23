project(wyagi)
cmake_minimum_required(VERSION 3.20)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED iostreams)

if(Boost_FOUND)
    include_directories(Project PUBLIC  ${Boost_INCLUDE_DIRS}) 
    add_subdirectory(fmt)
    set(CMAKE_CXX_FLAGS "--std=c++2a")

    set(Common_Sources utilities/Common.cpp utilities/SHA1.cpp utilities/Zlib.cpp)

    add_executable(wyagit main.cpp GitRepository.cpp ${Common_Sources}) 
    target_link_libraries(wyagit ${Boost_LIBRARIES} fmt)
endif()
