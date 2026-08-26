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

# if(NOT DEFINED ZEPHYR_NRFXLIB_MODULE_DIR)
#     message(FATAL_ERROR "ZEPHYR_NRFXLIB_MODULE_DIR must be passed via TFM_CMAKE_OPTIONS")
# endif()

set(OBERON_BASE ${ZEPHYR_NRFXLIB_MODULE_DIR}/crypto/nrf_oberon)
set(OBERON_INCLUDE_DIR ${OBERON_BASE}/include)
# if(NOT EXISTS "${OBERON_INCLUDE_DIR}/ocrypto_secp160r1.h")
#     message(FATAL_ERROR "Oberon headers not found at ${OBERON_INCLUDE_DIR}")
# endif()
target_include_directories(tfm_app_rot_partition_esp PRIVATE ${OBERON_INCLUDE_DIR})

# mbedcrypto_oberon_imported exists only when PSA_CRYPTO_DRIVER_OBERON is enabled.
# On CRACEN SoCs (e.g. nRF54L15) legacy ocrypto still lives in liboberon and must be linked directly.
# if(TARGET mbedcrypto_oberon_imported)
#     target_link_libraries(tfm_app_rot_partition_esp PRIVATE mbedcrypto_oberon_imported)
# else()
    include(${ZEPHYR_NRFXLIB_MODULE_DIR}/common.cmake)

    # TF-M SPE defaults to soft-float when CONFIG_TFM_ENABLE_FP is off.
    if(CONFIG_TFM_ENABLE_FP)
        set(CONFIG_FPU y)
        set(CONFIG_FP_HARDABI y)
    endif()

    if(CMAKE_C_FLAGS MATCHES "cortex-m33\\+nodsp")
        set(GCC_M_CPU cortex-m33+nodsp)
    elseif(NOT GCC_M_CPU)
        set(GCC_M_CPU ${TFM_SYSTEM_PROCESSOR})
    endif()

    set(OBERON_VER 3.0.19)
    nrfxlib_calculate_lib_path(OBERON_LIB_DIR
        BASE_DIR ${OBERON_BASE}
        SOFT_FLOAT_FALLBACK
    )
    set(OBERON_LIB ${OBERON_LIB_DIR}/liboberon_${OBERON_VER}.a)
    if(NOT EXISTS ${OBERON_LIB})
        message(FATAL_ERROR "liboberon not found at ${OBERON_LIB} (GCC_M_CPU=${GCC_M_CPU})")
    endif()
    target_link_libraries(tfm_app_rot_partition_esp PRIVATE ${OBERON_LIB})
# endif()