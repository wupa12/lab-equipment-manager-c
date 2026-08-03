cmake_minimum_required(VERSION 3.16)
project(lab_equipment_manager VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(lab_manager_core
    src/instrument.c
    src/input.c
    src/repository.c
)
target_include_directories(lab_manager_core PUBLIC include)

add_executable(lab-manager
    src/main.c
    src/cli.c
)
target_link_libraries(lab-manager PRIVATE lab_manager_core)

if(MSVC)
    target_compile_options(lab_manager_core PRIVATE /W4)
    target_compile_options(lab-manager PRIVATE /W4)
else()
    target_compile_options(lab_manager_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
    target_compile_options(lab-manager PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(lab-manager-tests tests/test_repository.c)
    target_link_libraries(lab-manager-tests PRIVATE lab_manager_core)
    if(MSVC)
        target_compile_options(lab-manager-tests PRIVATE /W4)
    else()
        target_compile_options(lab-manager-tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
    endif()
    add_test(NAME repository-tests COMMAND lab-manager-tests)
    set_tests_properties(repository-tests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
