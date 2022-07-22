cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall)

# Initialize the SDK
pico_sdk_init()

add_executable(main 
                main.c
                btle_driver.c
                btle_callbacks.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/hci_tl_interface.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/hci_tl.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/bluenrg1_l2cap_aci.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/bluenrg1_hci_le.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/bluenrg1_hal_aci.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/bluenrg1_gatt_aci.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/bluenrg1_gap_aci.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/bluenrg1_events.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/bluenrg1_events_cb.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/bluenrg1_devConfig.c
                ${CMAKE_CURRENT_SOURCE_DIR}/BlueNRG-2/ble_list.c
)

# pull in common dependencies
target_link_libraries(main pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(main)