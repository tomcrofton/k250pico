add_executable(k250interface)

pico_generate_pio_header(k250interface ${CMAKE_CURRENT_LIST_DIR}/ssarx.pio)
pico_generate_pio_header(k250interface ${CMAKE_CURRENT_LIST_DIR}/ssatx.pio)

target_sources(k250interface PRIVATE main.c)

target_link_libraries(k250interface PRIVATE pico_stdlib hardware_pio)

pico_enable_stdio_usb(k250interface 1)
pico_enable_stdio_uart(k250interface 0)

pico_add_extra_outputs(k250interface)
