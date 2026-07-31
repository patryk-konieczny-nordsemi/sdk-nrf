#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

# When MCUboot verifies the application image with a KMU-stored key (the
# "build_ns_boot" flavor), the public key must be provisioned into the KMU. This
# only happens when flashing with `west flash --erase` or `--recover`.
if(SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU)
  message(WARNING "
          ------------------------------------------------------------------------------
          --- MCUboot image verification uses a KMU-stored key. Flash with           ---
          --- `west flash -d build_ns_boot --domain provisioner --recover`           ---
          --- (or `--erase`) so the west runner provisions the MCUboot public        ---
          --- and the Fast Pair data in one operation to KMU and Protected Storage.  ---
          --- Then flash main application `west flash -d build_ns_boot`.             ---
          --- Wrong order of operations or skipping provisioning will cause errors   ---
          ------------------------------------------------------------------------------
          ")
endif()

# Apply the MCUboot flash layout explicitly. Auto-discovery of
# sysbuild/mcuboot/boards/<board>.overlay is unreliable here, so hand the MCUboot
# image the same partition map as the application (it reuses the app overlay and
# only re-points the code partition to the boot partition, and drops the flash
# deep-power-down timing that needs a system clock MCUboot does not have).
if(SB_CONFIG_BOOTLOADER_MCUBOOT AND "${BOARD}" MATCHES "nrf54l15")
  add_overlay_dts(mcuboot
    ${APP_DIR}/sysbuild/mcuboot/boards/nrf54l15dk_nrf54l15_cpuapp_ns_boot.overlay)
endif()

# The provisioner is only relevant for the nRF54L15 /ns (TF-M) target, where the
# Fast Pair registration data is sourced from hardware-protected storage.
if(SB_CONFIG_PROVISIONER_IMAGE AND "${BOARD}" MATCHES "nrf54l15")
  # The provisioner application is provided by the Fast Pair subsystem: the user
  # only maintains the main application. Enabling the provisioner image in
  # sysbuild pulls in the generated provisioner from the library.
  # NOTE: the provisioner cannot be added as APP_TYPE MAIN - sysbuild registers a
  # single set of global config targets (menuconfig/guiconfig/hardenconfig) per
  # MAIN image, so a second MAIN image collides with the default one. It is
  # therefore a plain BUILD_ONLY image and is signed explicitly below, with the
  # imgtool parameters derived from its own Kconfig (not hardcoded).
  ExternalZephyrProject_Add(
    APPLICATION provisioner
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/provision
    BUILD_ONLY TRUE
  )

  # Apply the *exact same* devicetree overlay as the input_device image so both
  # images see an identical flash partition layout. This guarantees TF-M's
  # ITS/PS/OTP secure-storage partitions live at the same addresses in both
  # images, so keys provisioned at runtime by the provisioner are found (and can
  # be cleared) by the input_device application. The layout differs between the
  # plain /ns build and the MCUboot-enabled /ns build, so pick the matching one.
  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    add_overlay_dts(provisioner
      ${APP_DIR}/boards/nrf54l15dk_nrf54l15_cpuapp_ns_boot.overlay)

    # Build the provisioner as an MCUboot image (so it links past the MCUboot
    # header at the primary slot). It shares the external-secondary-slot overlay,
    # so it also needs the SPI-NOR driver to enumerate that flash node.
    set_config_bool(provisioner CONFIG_BOOTLOADER_MCUBOOT y)
    set_config_bool(provisioner CONFIG_FLASH y)
    set_config_bool(provisioner CONFIG_SPI y)
    set_config_bool(provisioner CONFIG_SPI_NOR y)
  else()
    add_overlay_dts(provisioner
      ${APP_DIR}/boards/nrf54l15dk_nrf54l15_cpuapp_ns.overlay)
  endif()

  # Feed the Fast Pair credentials and storage locations (single source of truth
  # in Kconfig.sysbuild) into the provisioner image, and mirror the storage
  # locations into the application image so both agree on the KMU slot and the
  # Protected Storage uid.
  set_config_int(provisioner CONFIG_FP_PROVISION_MODEL_ID ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
  set_config_string(provisioner CONFIG_FP_PROVISION_ANTI_SPOOFING_KEY
    "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
  set_config_int(provisioner CONFIG_FP_PROVISION_KMU_SLOT ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(provisioner CONFIG_FP_PROVISION_PS_ID ${SB_CONFIG_BT_FAST_PAIR_PS_ID})

  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_KMU_SLOT ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PS_ID ${SB_CONFIG_BT_FAST_PAIR_PS_ID})

  # Build a single, self-contained provisioning image so the Fast Pair data can
  # be provisioned in ONE flash step:
  #   west flash -d <build> --domain provisioner --recover
  # That command erases the device, provisions the MCUboot public key into the
  # KMU (from keyfile.json, because of --recover) and programs the merged image
  # below. The provisioner then boots and writes the Anti-Spoofing key (KMU slot)
  # and Model ID (Protected Storage). The real application is flashed afterwards
  # as a plain update: `west flash -d <build>` (no --erase, so KMU/PS persist).
  #
  # Sign the provisioner for MCUboot and combine it with MCUboot into a single
  # flashable image, so the Fast Pair data can be provisioned in ONE step:
  #   west flash -d <build> --domain provisioner --recover
  # (erases, provisions the MCUboot KMU key from keyfile.json, and programs the
  # merged image; the real app is then flashed as a plain `west flash -d <build>`).
  #
  # The imgtool layout parameters are DERIVED (not hardcoded) so they follow the
  # devicetree overlay automatically:
  #   header-size = CONFIG_TFM_MCUBOOT_HEADER_SIZE (TF-M combined MCUboot header)
  #   rom-fixed / slot-size = read from the provisioner devicetree (slot0 base +
  #                           size and the slot0_ns offset) by sign_provisioner.cmake.
  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    set(prov_bin_dir ${CMAKE_BINARY_DIR}/provisioner/zephyr)
    set(prov_signed ${prov_bin_dir}/zephyr.signed.hex)
    set(prov_merged ${prov_bin_dir}/provisioning_merged.hex)
    set(mcuboot_hex ${CMAKE_BINARY_DIR}/mcuboot/zephyr/zephyr.hex)

    add_custom_command(
      OUTPUT ${prov_merged}
      # 1) Sign the provisioner's TF-M merged image. All layout-dependent params
      #    (header size, slot0 size, slot0_ns address) are read at build time from
      #    the provisioner's own .config and devicetree by the helper - nothing is
      #    hardcoded, so the signing follows the partition overlay automatically.
      COMMAND ${CMAKE_COMMAND}
        -DIMGTOOL=${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/imgtool.py
        -DPYTHON=${PYTHON_EXECUTABLE}
        -DDTS=${prov_bin_dir}/zephyr.dts
        -DCONFIG_FILE=${prov_bin_dir}/.config
        -DKEY=${APP_DIR}/sysbuild/boot_signature_key_file_ed25519.pem
        -DINPUT=${prov_bin_dir}/tfm_merged.hex
        -DOUTPUT=${prov_signed}
        -P ${APP_DIR}/sysbuild/sign_provisioner.cmake
      # 2) Merge MCUboot + the signed provisioner into one flashable hex.
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
        -o ${prov_merged} ${mcuboot_hex} ${prov_signed}
      # 3) Repoint the provisioner's west-flash runner at the merged hex.
      COMMAND ${CMAKE_COMMAND}
        -DRUNNERS=${prov_bin_dir}/runners.yaml -DMERGED=${prov_merged}
        -P ${APP_DIR}/sysbuild/repoint_provisioner_runner.cmake
      COMMENT "Fast Pair: building the provisioning image (MCUboot + signed provisioner)"
      VERBATIM
    )
    add_custom_target(fp_provisioner_provision_image ALL DEPENDS ${prov_merged})
    add_dependencies(fp_provisioner_provision_image provisioner mcuboot)
  endif()
endif()
