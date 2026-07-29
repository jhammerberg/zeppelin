# helium-lib: shared library code for the zeppelin workspace.
# Built as a Zephyr module library (see zephyr/module.yml).

zephyr_library()
zephyr_library_sources(src/udp_server.c)
zephyr_library_sources(src/time_sync.c)

zephyr_include_directories(include)
