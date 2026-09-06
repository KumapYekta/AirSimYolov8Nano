# Hem server hem client tarafından kullanılan sınıflar.
# Bagımlılıgı yalnızca GStreamer, AirSim ya da CUDA bilmez.

add_library(PipelineCommon STATIC
    src/GStreamerRuntimeManager.cpp
    src/FrameStatisticsManager.cpp)

add_library(AirSimYolov8::Common ALIAS PipelineCommon)

target_include_directories(PipelineCommon
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(PipelineCommon
    PUBLIC
        PkgConfig::GSTREAMER)

if(MSVC)
    target_compile_options(PipelineCommon PRIVATE
        /W4
        /wd4100
    )
else()
    target_compile_options(PipelineCommon PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
    )
endif()

set_target_properties(PipelineCommon PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
