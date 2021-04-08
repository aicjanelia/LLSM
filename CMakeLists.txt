cmake_minimum_required(VERSION 3.12...3.20)
project(llsm-pipeline VERSION 0.1 DESCRIPTION "Process lattice lightsheet images." LANGUAGES CXX)

######### Pre-Setup #########

# ITK wants us to use include before declaing any targets.
# Ref bottom of https://itk.org/Wiki/ITK/FAQ#NoFactoryException

find_package(ITK CONFIG REQUIRED ITKCommon ITKIOImageBase ITKIOTIFF ITKImageGrid ITKTransform ITKImageFunction)
include(${ITK_USE_FILE})

######### Targets #########

add_executable(deskew src/c/deskew/deskew.cpp)
add_executable(deskew-test src/c/tests/deskew-test.cpp)
add_executable(decon src/c/decon/decon.cpp)
add_executable(decon-test src/c/tests/decon-test.cpp)
add_executable(reader-test src/c/tests/reader-test.cpp)
add_executable(writer-test src/c/tests/writer-test.cpp)
add_executable(resampler-test src/c/tests/resampler-test.cpp)

set_property(TARGET deskew PROPERTY CXX_STANDARD 17)
set_property(TARGET deskew-test PROPERTY CXX_STANDARD 17)
set_property(TARGET decon PROPERTY CXX_STANDARD 17)
set_property(TARGET decon-test PROPERTY CXX_STANDARD 17)
set_property(TARGET reader-test PROPERTY CXX_STANDARD 17)
set_property(TARGET writer-test PROPERTY CXX_STANDARD 17)
set_property(TARGET resampler-test PROPERTY CXX_STANDARD 17)


######### Set Pre-package Variables #########

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

######### Add Packages #########

find_package(Boost 1.50 REQUIRED COMPONENTS filesystem program_options)

######### Includes #########

target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew)
target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
target_include_directories(deskew-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew)
target_include_directories(deskew-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/decon)
target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew)
target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
target_include_directories(decon-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/decon)
target_include_directories(decon-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew)
target_include_directories(decon-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(reader-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
target_include_directories(writer-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
target_include_directories(resampler-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

######### Libraries #########

target_link_libraries(deskew PRIVATE Boost::filesystem)
target_link_libraries(deskew PRIVATE Boost::program_options)
target_link_libraries(deskew PRIVATE ${ITK_LIBRARIES})
target_link_libraries(deskew-test PRIVATE Boost::filesystem)
target_link_libraries(deskew-test PRIVATE Boost::program_options)
target_link_libraries(deskew-test PRIVATE ${ITK_LIBRARIES})

target_link_libraries(decon PRIVATE Boost::filesystem)
target_link_libraries(decon PRIVATE Boost::program_options)
target_link_libraries(decon PRIVATE ${ITK_LIBRARIES})
target_link_libraries(decon-test PRIVATE Boost::filesystem)
target_link_libraries(decon-test PRIVATE Boost::program_options)
target_link_libraries(decon-test PRIVATE ${ITK_LIBRARIES})

target_link_libraries(reader-test PRIVATE Boost::filesystem)
target_link_libraries(reader-test PRIVATE ${ITK_LIBRARIES})

target_link_libraries(writer-test PRIVATE Boost::filesystem)
target_link_libraries(writer-test PRIVATE ${ITK_LIBRARIES})

target_link_libraries(resampler-test PRIVATE Boost::filesystem)
target_link_libraries(resampler-test PRIVATE ${ITK_LIBRARIES})

######### Installs #########

install(TARGETS deskew deskew-test decon reader-test writer-test resampler-test CONFIGURATIONS Release DESTINATION ${PROJECT_SOURCE_DIR}/bin)
