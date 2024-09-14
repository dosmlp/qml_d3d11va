cmake_minimum_required(VERSION 3.16)

project(qml_01 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.4 REQUIRED COMPONENTS Gui Widgets Quick Multimedia)
find_package(FFmpeg REQUIRED)

qt_standard_project_setup()

qt_add_executable(appqml_01
    main.cpp
    qffmpeg.h
    ffmpegplayer.h ffmpegplayer.cpp
    textureconverter.h textureconverter.cpp
    ffmpegvideobuffer.h ffmpegvideobuffer.cpp
    dvideooutput.h dvideooutput.cpp
)

qt_add_qml_module(appqml_01
    URI qml_01
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES d3d11va.h
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appqml_01 PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appqml_01
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include_directories(${FFMPEG_INCLUDE_DIRS})
target_link_directories(appqml_01 PRIVATE "D:/3rd/install/ffmpeg-x64/lib")
target_link_libraries(appqml_01
    PRIVATE Qt6::Quick Qt6::Multimedia Qt6::Gui Qt6::Widgets Qt::GuiPrivate
    bcrypt.lib
    Secur32.lib
    Mfuuid.lib
    Strmiids.lib
    libavcodec.lib
    libavformat.lib
    libavutil.lib
    libswresample.lib
    libswscale.lib
)

