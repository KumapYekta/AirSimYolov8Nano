# Client tarafı: Jetson Orin Nano. UDP'den RTP alır, nvv4l2decoder ile
# donanımda cozer, appsink cıktısını CUDA unified memory'ye tasır.

find_package(CUDAToolkit REQUIRED)

message(STATUS "CUDA        : ${CUDAToolkit_VERSION} (${CUDAToolkit_LIBRARY_ROOT})")

add_executable(JetsonFrameClient
    src/main.cpp
    src/ReceiverConfigProvider.cpp
    src/GStreamerReceiverPipelineManager.cpp
    src/CudaMemoryManager.cpp
    src/GpuFrameProvider.cpp)

target_include_directories(JetsonFrameClient
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(JetsonFrameClient
    PRIVATE
        AirSimYolov8::Common
        CUDA::cudart)

target_compile_options(JetsonFrameClient
    PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter)

install(TARGETS JetsonFrameClient RUNTIME DESTINATION bin)

# Ileride .cu kernel eklenince:
#   enable_language(CUDA)
#   set(CMAKE_CUDA_ARCHITECTURES 87)   # Orin
#   kaynak listesine src/kernels/*.cu eklenir
