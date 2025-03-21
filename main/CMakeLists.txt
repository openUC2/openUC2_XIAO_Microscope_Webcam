idf_component_register(SRCS "usb_webcam_main.c"
                    INCLUDE_DIRS ".")

include(gen_single_bin)



cmake_minimum_required(VERSION 3.5)
set(EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/examples/common_components/protocol_examples_common")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ota_webserver)
