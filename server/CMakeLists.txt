add_executable(server src/main.cpp)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(server PRIVATE 
    common
    Boost::serialization
    Boost::system
    Boost::thread
)
# Boost.Asio - header-only библиотека, не требует линковки
target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS})