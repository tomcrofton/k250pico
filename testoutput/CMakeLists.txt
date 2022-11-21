add_executable(pio_testout)

# by default the header is generated into the build dir
pico_generate_pio_header(pio_testout ${CMAKE_CURRENT_LIST_DIR}/testout.pio)
# however, alternatively you can choose to generate it somewhere else (in this case in the source tree for check in)
#pico_generate_pio_header(pio_testout ${CMAKE_CURRENT_LIST_DIR}/blink.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_sources(pio_testout PRIVATE testout.c)

target_link_libraries(pio_testout PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_testout)
