cmake_minimum_required(VERSION 3.22)

# Reported over BLE and by the `fw` shell command. Bump it for every build
# that is going to be pushed over the air, so each one is identifiable.
set(PROJECT_VER "1.1.2")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sf01_speaker)
