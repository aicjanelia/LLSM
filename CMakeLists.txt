cmake_minimum_required(VERSION 3.12...3.20)
project(llsm-pipeline VERSION 0.1 DESCRIPTION "Process lattice lightsheet images." LANGUAGES CXX)

######### Pre-Setup #########

# ITK wants us to use include before declaing any targets.
# Ref bottom of https://itk.org/Wiki/ITK/FAQ#NoFactoryException

find_package(ITK CONFIG REQUIRED ITKCommon ITKIOImageBase ITKIOTIFF ITKImageGrid ITKTransform ITKImageFunction)
include(${ITK_USE_FILE})

######### Targets #########

add_executable(deskew src/c/deskew/deskew.cpp)
add_executable(decon src/c/decon/decon.cpp)
add_executable(param-test src/c/tests/param-test.cpp)
add_executable(reader-test src/c/tests/reader-test.cpp)
add_executable(writer-test src/c/tests/writer-test.cpp)
add_executable(deskew-test src/c/tests/deskew-test.cpp)

######### Set Pre-package Variables #########

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

######### Add Packages #########

find_package(Boost 1.50 REQUIRED COMPONENTS filesystem program_options)

######### Includes #########

target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew)
target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/decon)
target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(param-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(reader-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(writer-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(deskew-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

######### Libraries #########

target_link_libraries(deskew PRIVATE Boost::filesystem)
target_link_libraries(deskew PRIVATE Boost::program_options)
target_link_libraries(deskew PRIVATE ITKCommon ITKIOImageBase ITKIOTIFF)

target_link_libraries(decon PRIVATE Boost::filesystem)
target_link_libraries(decon PRIVATE Boost::program_options)
target_link_libraries(decon PRIVATE ITKCommon ITKIOImageBase ITKIOTIFF)

target_link_libraries(param-test PRIVATE Boost::filesystem)
# target_link_libraries(param-test PRIVATE ITKCommon ITKIOImageBase ITKIOTIFF ITKImageGrid ITKTransform ITKImageFunction)
target_link_libraries(param-test PRIVATE ${ITK_LIBRARIES})

target_link_libraries(reader-test PRIVATE Boost::filesystem)
target_link_libraries(reader-test PRIVATE ITKCommon ITKIOImageBase ITKIOTIFF)

target_link_libraries(writer-test PRIVATE Boost::filesystem)
target_link_libraries(writer-test PRIVATE ITKCommon ITKIOImageBase ITKIOTIFF)

target_link_libraries(deskew-test PRIVATE Boost::filesystem)
target_link_libraries(deskew-test PRIVATE ${ITK_LIBRARIES})

######### Installs #########

install(TARGETS deskew decon param-test reader-test writer-test deskew-test CONFIGURATIONS Release DESTINATION ${PROJECT_SOURCE_DIR}/bin)
