add_library(tfm_app_rot_partition_esp STATIC)

target_include_directories(tfm_app_rot_partition_esp
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_BINARY_DIR}/example_secure_partition
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/example_secure_partition
)

target_sources(tfm_app_rot_partition_esp
    PRIVATE
        tfm_example_secure_partition.c
)

# The intermedia file defines the partition stack.
target_sources(tfm_app_rot_partition_esp
    PRIVATE
        ${CMAKE_BINARY_DIR}/example_secure_partition/auto_generated/intermedia_tfm_example_secure_partition.c
)

# The load info file includes the static data of the partition.
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/example_secure_partition/auto_generated/load_info_tfm_example_secure_partition.c
)

add_dependencies(tfm_app_rot_partition_esp manifest_tool)

target_link_libraries(tfm_app_rot_partition_esp
    PRIVATE
        platform_s
        tfm_sprt
        psa_crypto_config
)

target_link_libraries(tfm_spm
    PRIVATE
        tfm_app_rot_partition_esp
)

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_esp
)

target_compile_definitions(tfm_config 
    INTERFACE
        TFM_PARTITION_EXAMPLE_SECURE_PARTITION
)
