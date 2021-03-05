cmake_minimum_required(VERSION 3.12...3.19)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    message(FATAL_ERROR "A newer version of CMake is required. https://cmake.org/download/")
endif()

project(Deskew  VERSION         2.0 
                DESCRIPTION     "Deskew lattice lightsheet images." 
                LANGUAGES       CXX)


add_executable(deskew ${PROJECT_SOURCE_DIR}/src/c/deskew/deskew.cpp)
target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew ${PROJECT_SOURCE_DIR}/src/c/utils)

# Set the C++ standard
set(CMAKE_CXX_COMPILER "clang++")

# Add dependencies

## Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.50 REQUIRED COMPONENTS filesystem program_options)
message(STATUS "Boost version: ${Boost_VERSION}")
# This is needed if your Boost version is newer than your CMake version
# or if you have an old version of CMake (<3.5)
if(NOT TARGET Boost::filesystem)
    add_library(Boost::filesystem IMPORTED INTERFACE)
    set_property(TARGET Boost::filesystem PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})
    set_property(TARGET Boost::filesystem PROPERTY
        INTERFACE_LINK_LIBRARIES ${Boost_LIBRARIES})
endif()
target_link_libraries(deskew PUBLIC Boost::filesystem)

if(NOT TARGET Boost::program_options)
    add_library(Boost::program_options IMPORTED INTERFACE)
    set_property(TARGET Boost::program_options PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})
    set_property(TARGET Boost::program_options PROPERTY
        INTERFACE_LINK_LIBRARIES ${Boost_LIBRARIES})
endif()
target_link_libraries(deskew PUBLIC Boost::program_options)

## OpenMP
find_package(OpenMP REQUIRED)
message(STATUS "OpenMP version: ${OpenMP_CXX_VERSION} Path: ${OpenMP_CXX_LIBRARIES}")
target_link_libraries(deskew PRIVATE OpenMP::OpenMP_CXX)

## CImg
find_package(CImg REQUIRED)
message(STATUS "CImg include: ${CImg_DIR}")
# target_include_directories(libdeskew PRIVATE ${CImg_DIR}/../../include)
target_include_directories(deskew PRIVATE ${CImg_DIR}/../../include)

find_package(TIFF REQUIRED)
message(STATUS "Tiff version: ${TIFF_VERSION}")
set(CIMG_TIFF_CCFLAGS -Dcimg_use_tiff)
SET(CIMG_CFLAGS "${CIMG_CFLAGS} ${CIMG_TIFF_CCFLAGS}")
target_include_directories(deskew PRIVATE ${TIFF_INCLUDE_DIR})
target_link_libraries(deskew PRIVATE ${TIFF_LIBRARIES})

find_package(JPEG REQUIRED)
message(STATUS "JPEG version: ${JPEG_VERSION}")
set(CIMG_JPEG_CCFLAGS -Dcimg_use_jpeg)
SET(CIMG_CFLAGS "${CIMG_CFLAGS} ${CIMG_JPEG_CCFLAGS}")
target_include_directories(deskew PRIVATE ${JPEG_LIB_DIRS})
target_link_libraries(deskew PRIVATE ${JPEG_LIBRARIES})

find_package(ZLIB REQUIRED)
message(STATUS "ZLIB version: ${ZLIB_VERSION}")
SET(CIMG_CFLAGS "${CIMG_CFLAGS} ${CIMG_ZLIB_CCFLAGS}")
target_include_directories(deskew PRIVATE ${ZLIB_LIB_DIRS})
target_link_libraries(deskew PRIVATE ${ZLIB_LIBRARIES})

install(TARGETS deskew CONFIGURATIONS Release DESTINATION ${PROJECT_SOURCE_DIR}/bin)
