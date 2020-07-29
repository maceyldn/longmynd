cmake_minimum_required(VERSION 2.8.9)
project (longmynd)
add_executable(longmynd  main.c nim.c ftdi.c stv0910.c stv0910_utils.c stvvglna.c stvvglna_utils.c stv6120.c stv6120_utils.c ftdi_usb.c fifo.c udp.c udp_rcv.c beep.c ts.c)
add_executable(fake_read  fake_read.c)

install(TARGETS longmynd fake_read DESTINATION bin)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  )


find_package (Threads REQUIRED)
find_package (libusb-1.0 REQUIRED)
find_package (ALSA REQUIRED)

target_link_libraries (longmynd pthread usb-1.0 asound m )
