cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ src)
add_executable(mbus_event_test ${src})


target_link_libraries(mbus_event_test PRIVATE  xxbase xmbus gtest gmock rocksdb zstd snappy bz2 z)
