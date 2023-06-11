project(wyagi)
cmake_minimum_required(VERSION 3.20)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED iostreams program_options)
include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG a0b8a92
)
FetchContent_MakeAvailable(fmt)

if(Boost_FOUND)
    include_directories(Project PUBLIC  ${Boost_INCLUDE_DIRS}) 
    set(CMAKE_CXX_FLAGS "--std=c++2a")

    set(Objects_Sources git_objects/GitObject.cpp 
                        git_objects/GitRepository.cpp 
                        git_objects/GitHash.cpp
                        git_objects/GitObjectsFactory.cpp
                        git_objects/GitIndex.cpp)

    set(Common_Sources utilities/Common.cpp
                       utilities/SHA1.cpp
                       utilities/Zlib.cpp)

    add_executable(wyagit main.cpp  ${Common_Sources} ${Objects_Sources}) 
    target_link_libraries(wyagit ${Boost_LIBRARIES} fmt)
endif()
