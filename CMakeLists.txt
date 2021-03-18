cmake_minimum_required(VERSION 3.12...3.20)
project(llsm-pipeline VERSION 0.1 DESCRIPTION "Process lattice lightsheet images." LANGUAGES CXX)

######### Pre-Setup #########

# ITK wants us to use include before declaing any targets.
# Ref bottom of https://itk.org/Wiki/ITK/FAQ#NoFactoryException

find_package(ITK CONFIG REQUIRED ITKCommon ITKIOImageBase ITKIOTIFF)
include(${ITK_USE_FILE})

######### Targets #########

add_executable(deskew src/c/deskew/deskew.cpp)
add_executable(decon src/c/decon/decon.cpp)
add_executable(reader-test src/c/tests/readerTest.cpp)

######### Set Pre-package Variables #########

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

######### Add Packages #########

find_package(Boost 1.50 REQUIRED COMPONENTS filesystem program_options)
find_package(OpenMP REQUIRED)
find_package(CImg REQUIRED)
find_package(TIFF REQUIRED)
find_package(JPEG REQUIRED)
find_package(ZLIB REQUIRED)

######### Set Post-package Variables #########

set(CIMG_TIFF_CCFLAGS -Dcimg_use_tiff)
set(CIMG_CFLAGS "${CIMG_CFLAGS} ${CIMG_TIFF_CCFLAGS}")
set(CIMG_JPEG_CCFLAGS -Dcimg_use_jpeg)
SET(CIMG_CFLAGS "${CIMG_CFLAGS} ${CIMG_JPEG_CCFLAGS}")
SET(CIMG_CFLAGS "${CIMG_CFLAGS} ${CIMG_ZLIB_CCFLAGS}")

######### Includes #########

target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew)
target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
target_include_directories(deskew PRIVATE ${TIFF_INCLUDE_DIR})
target_include_directories(deskew PRIVATE ${CImg_INCLUDE_DIR})
target_include_directories(deskew PRIVATE ${JPEG_LIB_DIRS})

target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/decon)
target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
target_include_directories(decon PRIVATE ${TIFF_INCLUDE_DIR}) # TODO: remove when we kill GetBitDepth in utils

target_include_directories(reader-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/tests)
target_include_directories(reader-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

######### Libraries #########

target_link_libraries(deskew PRIVATE Boost::filesystem)
target_link_libraries(deskew PRIVATE Boost::program_options)
target_link_libraries(deskew PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(deskew PRIVATE ${TIFF_LIBRARIES})
target_link_libraries(deskew PRIVATE ${JPEG_LIBRARIES})
target_link_libraries(deskew PRIVATE ZLIB::ZLIB)

target_link_libraries(decon PRIVATE Boost::filesystem)
target_link_libraries(decon PRIVATE Boost::program_options)
target_link_libraries(decon PRIVATE ${TIFF_LIBRARIES}) # TODO: remove when we kill GetBitDepth in utils
target_link_libraries(decon PRIVATE ITKCommon)

target_link_libraries(reader-test PRIVATE ZLIB::ZLIB)
target_link_libraries(reader-test PRIVATE ITKCommon ITKIOImageBase ITKIOTIFF)

######### Installs #########

install(TARGETS deskew decon reader-test CONFIGURATIONS Release DESTINATION ${PROJECT_SOURCE_DIR}/bin)
