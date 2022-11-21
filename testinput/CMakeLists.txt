add_executable(pio_testinput)

pico_generate_pio_header(pio_testinput ${CMAKE_CURRENT_LIST_DIR}/testinput.pio)

target_sources(pio_testinput PRIVATE testinput.c)

target_link_libraries(pio_testinput PRIVATE pico_stdlib hardware_pio)

pico_enable_stdio_usb(pio_testinput 1)
pico_enable_stdio_uart(pio_testinput 0)

pico_add_extra_outputs(pio_testinput)
