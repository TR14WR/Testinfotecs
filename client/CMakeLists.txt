add_executable(client src/main.cpp)
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(client PRIVATE 
    common
    Boost::serialization
    Boost::system
    Boost::thread
)
# Boost.Asio - header-only библиотека, не требует линковки
target_include_directories(client PRIVATE ${Boost_INCLUDE_DIRS})