#include "ffmpegplayer.h"
#include <QDebug>
#include <QThreadPool>
#include <QRunnable>
#include <QtMultimedia/QVideoSink>
#include "ffmpegvideobuffer.h"
#include "textureconverter.h"

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    AVPixelFormat hw = ((FFmpegPlayer*)ctx->opaque)->pixFmt();
    const enum AVPixelFormat *p;
    D3D11TextureConverter::SetupDecoderTextures(ctx);

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

int FFmpegPlayer::hwDecoderInit(AVCodecContext *ctx, const enum AVHWDeviceType type, int64_t d3d11device)
{
    int err = 0;
    AVDictionary* dic = nullptr;
    av_dict_set_int(&dic,"d3d11device",d3d11device,0);
    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, dic, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    av_dict_free(&dic);
    return err;
}

int FFmpegPlayer::readAndDecode()
{
    int ret = 0;
    while (ret >= 0) {
        if ((ret = av_read_frame(input_ctx, packet)) < 0) {
            qWarning()<<"end";
            break;
        }


        if (video_stream == packet->stream_index) {
            ret = avcodec_send_packet(decoder_ctx, packet);
            if (ret < 0) {
                qWarning()<<"Error during decoding";
                return ret;
            }
            // AVFrame* frame = nullptr;

            while (1) {
                AVFrameUPtr frame = makeAVFrame();
                int ret2 = avcodec_receive_frame(decoder_ctx, frame.get());
                if (ret2 == AVERROR_EOF) {
                    return 0;
                } else if (ret2 == AVERROR(EAGAIN)) {
                    break;
                } else if (ret2 < 0) {
                    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    qWarning()<<av_make_error_string(buf,AV_ERROR_MAX_STRING_SIZE,ret2);
                    return 0;
                }

                if (frame->format == hw_pix_fmt) {
                    if (tc_.isNull()) {
                        tc_ = TextureConverter(m_videoSink->rhi());
                    }
                    qint64 pts = frame->pts;
                    qint64 endtime = frame->pts+frame->duration;
                    auto buffer = std::make_unique<DFFmpegVideoBuffer>(std::move(frame));
                    buffer->setTextureConverter(tc_);
                    QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
                    format.setColorSpace(buffer->colorSpace());
                    format.setColorTransfer(buffer->colorTransfer());
                    format.setColorRange(buffer->colorRange());
                    format.setMaxLuminance(buffer->maxNits());
                    QVideoFrame videoFrame(buffer.release(), format);
                    videoFrame.setStartTime(pts);
                    videoFrame.setEndTime(endtime);
                    videoFrame.setRotation(QtVideo::Rotation::None);
                    m_videoSink->setVideoFrame(videoFrame);
                }
                // Sleep(1);
            }
        }


        av_packet_unref(packet);
    }
}

FFmpegPlayer::FFmpegPlayer(QObject *parent)
    : QObject{parent}
{}

int FFmpegPlayer::open(const QString &file)
{
    type = AV_HWDEVICE_TYPE_D3D11VA;
    packet = av_packet_alloc();
    if (!packet) {
        fprintf(stderr, "Failed to allocate AVPacket\n");
        return -1;
    }

    /* open the input file */
    if (avformat_open_input(&input_ctx, file.mid(8).toUtf8().data(), NULL, NULL) != 0) {
        qWarning()<<"Cannot open input file"<<file;
        return -1;
    }

    if (avformat_find_stream_info(input_ctx, NULL) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        qWarning()<<"Cannot find input stream information.";
        return -1;
    }
    /* find the video stream information */
    ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        qWarning()<<"Cannot find a video stream in the input file";
        return -1;
    }
    video_stream = ret;

    for (int i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
        if (!config) {
            qWarning()<<"Decoder "<<decoder->name<<" does not support device type "<<av_hwdevice_get_type_name(type);
            return -1;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
        return AVERROR(ENOMEM);

    video = input_ctx->streams[video_stream];
    if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
        return -1;

    decoder_ctx->opaque = this;
    decoder_ctx->get_format  = get_hw_format;

    onRhiChange(m_videoSink->rhi());

    hwDecoderInit(decoder_ctx,type,d3d11_device_);

    if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
        qWarning()<<"Failed to open codec for stream";
        return -1;
    }
    QThreadPool::globalInstance()->start(std::bind(&FFmpegPlayer::readAndDecode,this));
}

AVPixelFormat FFmpegPlayer::pixFmt() const
{
    return hw_pix_fmt;
}

QVideoSink *FFmpegPlayer::videoSink() const
{
    return m_videoSink;
}

void FFmpegPlayer::setVideoSink(QVideoSink *videoSink)
{
    if (m_videoSink == videoSink)
        return;
    m_videoSink = videoSink;
    emit videoSinkChanged();
}

void FFmpegPlayer::onRhiChange(QRhi *rhi)
{
    d3d11_device_ = (int64_t)((QRhiD3D11NativeHandles*)m_videoSink->rhi()->nativeHandles())->dev;
}
/*
    const auto pixelAspectRatio = codec->pixelAspectRatio(frame.avFrame());
*/

