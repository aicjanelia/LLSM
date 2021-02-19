cmake_minimum_required(VERSION 3.18)

# set the project name
project(Deconv)

# ITK
find_package(ITK REQUIRED
    COMPONENTS
    MorphologicalContourInterpolation
    ITKSmoothing
    ITKIOImageBase
    ITKIONRRD
)
include(${ITK_USE_FILE})

# # add the executable
# add_executable(deconv deconv.cpp)
add_executable(deconv hw.cpp)

target_link_libraries(deconv ${ITK_LIBRARIES})

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)