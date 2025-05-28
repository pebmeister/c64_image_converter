cmake_minimum_required(VERSION 3.20)
project(c64_image_converter)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure output directories for Visual Studio
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source files
set(SOURCE_FILES 
    src/main.cpp 
    src/dither.cpp
    src/dither.h
    src/preview.cpp
    src/preview.h
    src/pallet.cpp
    src/pallet.h
    src/scale.cpp
    src/scale.h
    src/blockreducer.cpp
    src/blockreducer.h
    )

# Executable
add_executable(c64_converter ${SOURCE_FILES})

# STB configuration
include(FetchContent)
FetchContent_Declare(
    stb
    URL https://github.com/nothings/stb/archive/master.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(stb)
target_include_directories(c64_converter PRIVATE ${stb_SOURCE_DIR})
target_compile_definitions(c64_converter PRIVATE  # Fixed typo here (was c64_conporter)
    STB_IMAGE_IMPLEMENTATION
    STB_IMAGE_WRITE_IMPLEMENTATION
)

# SFML configuration
option(ENABLE_SFML_PREVIEW "Enable SFML preview" ON)

if(ENABLE_SFML_PREVIEW)
    # Try to find system SFML first
    find_package(SFML 2.5 COMPONENTS graphics window system QUIET)

    if(NOT SFML_FOUND)
        message(STATUS "SFML not found in system, downloading from GitHub")
        # Download SFML if not found
        FetchContent_Declare(
            sfml
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 2.5.1
            GIT_SHALLOW TRUE
        )
        
        set(SFML_BUILD_WINDOW ON)
        set(SFML_BUILD_GRAPHICS ON)
        set(SFML_BUILD_SYSTEM ON)
        set(SFML_BUILD_AUDIO OFF)
        set(SFML_BUILD_NETWORK OFF)
        
        FetchContent_MakeAvailable(sfml)
    endif()

    if(SFML_FOUND OR TARGET sfml-graphics)
        target_compile_definitions(c64_converter PRIVATE USE_SFML)
        
        if(SFML_FOUND)
            message(STATUS "Using system SFML installation")
            target_link_libraries(c64_converter PRIVATE 
                SFML::Graphics 
                SFML::Window 
                SFML::System
            )
        else()
            message(STATUS "Using downloaded SFML version")
            target_link_libraries(c64_converter PRIVATE 
                sfml-graphics 
                sfml-window 
                sfml-system
            )
        endif()

        # Copy DLLs to output directory on Windows

        if(WIN32)
            set_target_properties(c64_converter PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
                RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
            )

            foreach(lib sfml-graphics sfml-window sfml-system)
                if(TARGET ${lib})
                    add_custom_command(TARGET c64_converter POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            $<TARGET_FILE:${lib}>
                            $<TARGET_FILE_DIR:c64_converter>
                    )
                endif()
            endforeach()
        endif()    

    else()
        message(WARNING "SFML not found - preview will be disabled")
    endif()
    
endif()

# Install target
install(TARGETS c64_converter DESTINATION bin)