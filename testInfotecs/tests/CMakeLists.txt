# Включаем тестирование только если доступен gtest
find_package(GTest QUIET)

if(GTest_FOUND)
    add_executable(integration_tests
        test_integration.cpp
    )
    
    target_include_directories(integration_tests PRIVATE
        ../common
    )
    
    target_link_libraries(integration_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
    )
    
    add_test(NAME IntegrationTests COMMAND integration_tests)
else()
    message(WARNING "GTest not found. Tests will not be built. Install GTest or disable BUILD_TESTS option.")
endif()
