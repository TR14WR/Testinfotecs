add_executable(client src/main.cpp)
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(client PRIVATE 
    common
    Boost::asio
    Boost::serialization
    Boost::system
)
