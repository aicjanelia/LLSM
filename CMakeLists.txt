cmake_minimum_required(VERSION 3.12...3.20)
project(llsm-pipeline VERSION 0.1 DESCRIPTION "Process lattice lightsheet images." LANGUAGES CXX)

######### Build Configuration #########

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug Release RelWithDebInfo MinSizeRel)" FORCE)
endif()

# Compiler optimization flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Aggressive optimization flags for maximum performance
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops")
  
  # Enable Link Time Optimization (LTO) for even better performance
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endif()

######### Pre-Setup #########

# ITK wants us to use include before declaing any targets.
# Ref bottom of https://itk.org/Wiki/ITK/FAQ#NoFactoryException

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

# Check if ITK was built with FFTW support
message(STATUS "ITK Version: ${ITK_VERSION}")
message(STATUS "ITK_USE_FFTWF: ${ITK_USE_FFTWF}")
message(STATUS "ITK_USE_FFTWD: ${ITK_USE_FFTWD}")
if(ITK_USE_FFTWF OR ITK_USE_FFTWD)
  message(STATUS "FFTW support detected in ITK - multi-threaded deconvolution enabled")
else()
  message(WARNING "FFTW support NOT detected in ITK - deconvolution will be single-threaded")
  message(WARNING "To enable multi-threading, rebuild ITK with:")
  message(WARNING "  -DITK_USE_FFTWF=ON -DITK_USE_FFTWD=ON")
endif()

######### Targets #########

add_executable(flatfield src/c/flatfield/flatfield.cpp)
add_executable(crop src/c/crop/crop.cpp)
add_executable(deskew src/c/deskew/deskew.cpp)
# add_executable(deskew-test src/c/tests/deskew-test.cpp)
add_executable(decon src/c/decon/decon.cpp)
# add_executable(decon-test src/c/tests/decon-test.cpp)
add_executable(mip src/c/mip/mip.cpp)
# add_executable(mip-test src/c/tests/mip-test.cpp)
# add_executable(reader-test src/c/tests/reader-test.cpp)
# add_executable(writer-test src/c/tests/writer-test.cpp)
# add_executable(resampler-test src/c/tests/resampler-test.cpp)
add_executable(check_itk_fftw check_itk_fftw.cpp)

set_property(TARGET flatfield PROPERTY CXX_STANDARD 14)
set_property(TARGET flatfield PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET flatfield PROPERTY CMAKE_CXX_EXTENSIONS  OFF)
set_property(TARGET crop PROPERTY CXX_STANDARD 14)
set_property(TARGET crop PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET crop PROPERTY CMAKE_CXX_EXTENSIONS  OFF)
set_property(TARGET deskew PROPERTY CXX_STANDARD 14)
set_property(TARGET deskew PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET deskew PROPERTY CMAKE_CXX_EXTENSIONS  OFF)
# set_property(TARGET deskew-test PROPERTY CXX_STANDARD 17)
set_property(TARGET decon PROPERTY CXX_STANDARD 14)
set_property(TARGET decon PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET decon PROPERTY CMAKE_CXX_EXTENSIONS  OFF)
# set_property(TARGET decon-test PROPERTY CXX_STANDARD 17)
set_property(TARGET mip PROPERTY CXX_STANDARD 14)
set_property(TARGET mip PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET mip PROPERTY CMAKE_CXX_EXTENSIONS  OFF)
# set_property(TARGET mip-test PROPERTY CXX_STANDARD 17)
# set_property(TARGET reader-test PROPERTY CXX_STANDARD 17)
# set_property(TARGET writer-test PROPERTY CXX_STANDARD 17)
# set_property(TARGET resampler-test PROPERTY CXX_STANDARD 17)
set_property(TARGET check_itk_fftw PROPERTY CXX_STANDARD 14)
set_property(TARGET check_itk_fftw PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET check_itk_fftw PROPERTY CMAKE_CXX_EXTENSIONS  OFF)

######### Set Pre-package Variables #########

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

######### Add Packages #########

find_package(Boost 1.50 REQUIRED COMPONENTS filesystem program_options)

######### Includes #########

target_include_directories(flatfield PRIVATE ${PROJECT_SOURCE_DIR}/src/c/flatfield)
target_include_directories(flatfield PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(crop PRIVATE ${PROJECT_SOURCE_DIR}/src/c/crop)
target_include_directories(crop PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew)
target_include_directories(deskew PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
# target_include_directories(deskew-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/deskew)
# target_include_directories(deskew-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/decon)
target_include_directories(decon PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
# target_include_directories(decon-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/decon)
# target_include_directories(decon-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

target_include_directories(mip PRIVATE ${PROJECT_SOURCE_DIR}/src/c/mip)
target_include_directories(mip PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
# target_include_directories(mip-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/mip)
# target_include_directories(mip-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

# target_include_directories(reader-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
# target_include_directories(writer-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)
# target_include_directories(resampler-test PRIVATE ${PROJECT_SOURCE_DIR}/src/c/utils)

######### Libraries #########

target_link_libraries(flatfield PRIVATE Boost::filesystem)
target_link_libraries(flatfield PRIVATE Boost::program_options)
target_link_libraries(flatfield PRIVATE ${ITK_LIBRARIES})

target_link_libraries(crop PRIVATE Boost::filesystem)
target_link_libraries(crop PRIVATE Boost::program_options)
target_link_libraries(crop PRIVATE ${ITK_LIBRARIES})

target_link_libraries(deskew PRIVATE Boost::filesystem)
target_link_libraries(deskew PRIVATE Boost::program_options)
target_link_libraries(deskew PRIVATE ${ITK_LIBRARIES})
# target_link_libraries(deskew-test PRIVATE Boost::filesystem)
# target_link_libraries(deskew-test PRIVATE Boost::program_options)
# target_link_libraries(deskew-test PRIVATE ${ITK_LIBRARIES})

target_link_libraries(decon PRIVATE Boost::filesystem)
target_link_libraries(decon PRIVATE Boost::program_options)
target_link_libraries(decon PRIVATE ${ITK_LIBRARIES})
# target_link_libraries(decon-test PRIVATE Boost::filesystem)
# target_link_libraries(decon-test PRIVATE Boost::program_options)
# target_link_libraries(decon-test PRIVATE ${ITK_LIBRARIES})

target_link_libraries(mip PRIVATE Boost::filesystem)
target_link_libraries(mip PRIVATE Boost::program_options)
target_link_libraries(mip PRIVATE ${ITK_LIBRARIES})
# target_link_libraries(mip-test PRIVATE Boost::filesystem)
# target_link_libraries(mip-test PRIVATE Boost::program_options)
# target_link_libraries(mip-test PRIVATE ${ITK_LIBRARIES})

# target_link_libraries(reader-test PRIVATE Boost::filesystem)
# target_link_libraries(reader-test PRIVATE ${ITK_LIBRARIES})

# target_link_libraries(writer-test PRIVATE Boost::filesystem)
# target_link_libraries(writer-test PRIVATE ${ITK_LIBRARIES})

# target_link_libraries(resampler-test PRIVATE Boost::filesystem)
# target_link_libraries(resampler-test PRIVATE ${ITK_LIBRARIES})

target_link_libraries(check_itk_fftw PRIVATE ${ITK_LIBRARIES})

######### Installs #########

# install(TARGETS deskew deskew-test decon decon-test mip mip-test reader-test writer-test resampler-test CONFIGURATIONS Release DESTINATION ${PROJECT_SOURCE_DIR}/bin)
install(TARGETS flatfield crop deskew decon mip check_itk_fftw CONFIGURATIONS Release DESTINATION ${PROJECT_SOURCE_DIR}/bin)

file(COPY ${PROJECT_SOURCE_DIR}/src/python/llsm-pipeline.py DESTINATION ${PROJECT_SOURCE_DIR}/bin)
file(COPY ${PROJECT_SOURCE_DIR}/src/python/settings2json.py DESTINATION ${PROJECT_SOURCE_DIR}/bin)

