cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Build only the application, BSP, and their dependency closure. The product is
# offline and intentionally does not pull in unrelated network/demo components.
idf_build_set_property(MINIMAL_BUILD ON)

project(FoloToy-AI-Passport)
