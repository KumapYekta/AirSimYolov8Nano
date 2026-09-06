# Server tarafı: AirSim'den goruntu ceker, GStreamer ile UDP'ye basar.
# AMD 6700XT uzerinde VAAPI donanım encode kullanır.

find_package(AirSim REQUIRED)

add_executable(AirSimStreamServer
    src/main.cpp
    src/AppConfigProvider.cpp
    src/AirSimConnectionManager.cpp
    src/ImageRequestProvider.cpp
    src/CameraImageProvider.cpp
    src/GStreamerSenderPipelineManager.cpp
    src/AppSrcFrameProvider.cpp)

target_include_directories(AirSimStreamServer
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(AirSimStreamServer
    PRIVATE
        AirSimYolov8::Common
        AirSim::AirLib)

if(MSVC)
    target_compile_options(AirSimStreamServer PRIVATE
        /W4
        /wd4100 # -Wno-unused-parameter dengi: unreferenced formal parameter uyarısını kapatır
    )
else()
    target_compile_options(AirSimStreamServer PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
    )
endif()

# AirLib clang + libc++ ile derlendiyse uygulamayı da aynı standart
# kutuphaneyle linklemek gerekir, yoksa std::__1 sembol hataları gelir.
option(AIRSIM_USE_LIBCXX "Link against libc++ (required if AirLib was built with clang/libc++)" OFF)

if(AIRSIM_USE_LIBCXX)
    target_compile_options(AirSimStreamServer PRIVATE -stdlib=libc++)
    target_link_options(AirSimStreamServer PRIVATE -stdlib=libc++)
endif()

install(TARGETS AirSimStreamServer RUNTIME DESTINATION bin)
