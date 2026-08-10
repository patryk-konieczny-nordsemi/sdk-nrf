# Provisioner library wiring (included from the user application CMakeLists after
# project()). Adds core sources and linker setup only — the user project supplies
# its own main (e.g. provisioner/provisioner_main.c) that calls provision_data().

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/include)

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/src/provisioner.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fp_register_provision_data.c)

zephyr_linker_sources(SECTIONS
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/provision_entry.ld)
