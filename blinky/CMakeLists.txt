add_executable(blinky
        blinky.c
        )

# pull in common dependencies
target_link_libraries(blinky pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(blinky)

# add url via pico_set_program_url
#example_auto_set_url(blinky)