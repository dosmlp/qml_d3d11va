#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QQuickWindow>
#include <QDebug>
#include <QQuickItem>
#include "ffmpegplayer.h"
#include <qt_windows.h>
#include <tchar.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    qmlRegisterType<FFmpegPlayer>("FFmpegPlayer",1, 0, "FFmpegPlayer");

    QQmlApplicationEngine engine;

    const QUrl url(QStringLiteral("qrc:/qml_01/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);
    // 获取顶层QML对象
    auto topLevel = engine.rootObjects();

    //FFmpegPlayer player;
    //player.open("D:/1506xzsNlx07MXDGQyFW01041200U0AM0E010.mp4");

    return app.exec();
}
