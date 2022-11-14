add_executable(serialtest
        main_ser.c
        )

# pull in common dependencies
target_link_libraries(serialtest pico_stdlib)

pico_enable_stdio_usb(serialtest 1)
pico_enable_stdio_uart(serialtest 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(serialtest)

