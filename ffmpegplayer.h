#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QPointer>
#include <QQmlEngine>
#include "qffmpeg.h"
#include "textureconverter.h"


class FFmpegPlayer : public QObject
{
    Q_OBJECT
    //QML_NAMED_ELEMENT(FFmpegPlayer)
    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
public:
    explicit FFmpegPlayer(QObject *parent = nullptr);
    Q_INVOKABLE int open(const QString& file);
    AVPixelFormat pixFmt() const;
    QVideoSink* videoSink() const;
    void setVideoSink(QVideoSink *videoSink);
    void onRhiChange(QRhi* rhi);
signals:
    void videoSinkChanged();
    void viviChanged();
private:
    int hwDecoderInit(AVCodecContext *ctx, const enum AVHWDeviceType type, int64_t d3d11device = 0);
    int readAndDecode();
    AVFormatContext *input_ctx = NULL;
    int video_stream, ret;
    AVStream *video = NULL;
    AVCodecContext *decoder_ctx = NULL;
    const AVCodec *decoder = NULL;
    AVPacket *packet = NULL;
    AVBufferRef *hw_device_ctx = NULL;
    enum AVHWDeviceType type;
    enum AVPixelFormat hw_pix_fmt;
    QVideoSink* m_videoSink = nullptr;

    TextureConverter tc_;
    int64_t d3d11_device_ = 0;
};

#endif // FFMPEGPLAYER_H
