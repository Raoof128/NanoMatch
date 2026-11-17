find_package(GTest REQUIRED)

set(TEST_SOURCES
    test_order.cpp
    test_price_level.cpp
    test_book_side.cpp
    test_order_book.cpp
    test_matching_limit.cpp
    test_matching_market.cpp
    test_matching_ioc.cpp
    test_matching_fok.cpp
    test_cancel_modify.cpp
    test_edge_cases.cpp
    test_pool_allocator.cpp
    test_csv_reader.cpp
    test_csv_writer.cpp
    test_order_generator.cpp
    test_latency_stats.cpp
    test_throughput_stats.cpp
    test_order_validator.cpp
    test_matching_engine.cpp
    test_regressions.cpp
    test_invariants.cpp
)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE nanomatch_lib GTest::GTest GTest::Main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
