project(shared_memory_socket)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wall -pedantic -std=c++14 -g3 -O0")
#add_executable(server main.cpp c_turbosocket.cpp c_counter.cpp)
#add_executable(client client.cpp c_turbosocket.cpp c_counter.cpp)
#target_link_libraries(server boost_system pthread rt)
#target_link_libraries(client boost_system pthread rt)

#add_executable(msg_queue_client msg_queue_client.cpp c_counter.cpp)
#add_executable(msg_queue_server msg_queue_server.cpp c_counter.cpp)
#target_link_libraries(msg_queue_client pthread rt)
#target_link_libraries(msg_queue_server pthread rt)

add_library(preload SHARED preload.cpp c_turbosocket.cpp c_sockfd_manager.cpp)
target_link_libraries(preload boost_system dl pthread rt)


add_executable(preload_server preload_server.cpp c_turbosocket.cpp c_sockfd_manager.cpp)
target_link_libraries(preload_server boost_system dl pthread rt)
