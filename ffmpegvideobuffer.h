#ifndef FFMPEGVIDEOBUFFER_H
#define FFMPEGVIDEOBUFFER_H

#include <QObject>
#include <QVideoFrame>
#include <QMatrix4x4>
#include "qffmpeg.h"
#include "textureconverter.h"

#include <qt_windows.h>



class QVideoFrameTextures
{
public:
    virtual ~QVideoFrameTextures() {}
    virtual QRhiTexture *texture(uint plane) const = 0;
};

class QAbstractVideoBuffer
{
public:
    QAbstractVideoBuffer(QVideoFrame::HandleType type, QRhi *rhi = nullptr):
        m_type(type),
        m_rhi(rhi)
    {}
    virtual ~QAbstractVideoBuffer(){}

    QVideoFrame::HandleType handleType() const { return m_type; }
    QRhi *rhi() const { return m_rhi; }

    struct MapData
    {
        int nPlanes = 0;
        int bytesPerLine[4] = {};
        uchar *data[4] = {};
        int size[4] = {};
    };

    virtual MapData map(QVideoFrame::MapMode mode) = 0;
    virtual void unmap() = 0;

    virtual std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi *) { return {}; }
    virtual quint64 textureHandle(QRhi *, int /*plane*/) const { return 0; }

    virtual QMatrix4x4 externalTextureMatrix() const { return {}; }

protected:
    QVideoFrame::HandleType m_type;
    QRhi *m_rhi = nullptr;

private:
    Q_DISABLE_COPY(QAbstractVideoBuffer)
};

class DFFmpegVideoBuffer : public QAbstractVideoBuffer
{
public:
    DFFmpegVideoBuffer(AVFrameUPtr frame);
    ~DFFmpegVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    virtual std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi *) override;
    virtual quint64 textureHandle(QRhi *rhi, int plane) const override;

    QVideoFrameFormat::PixelFormat pixelFormat() const;
    QSize size() const;

    static QVideoFrameFormat::PixelFormat toQtPixelFormat(AVPixelFormat avPixelFormat, bool *needsConversion = nullptr);
    static AVPixelFormat toAVPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat);

    void convertSWFrame();

    AVFrame *getHWFrame() const { return m_hwFrame.get(); }

    void setTextureConverter(const TextureConverter &converter);

    QVideoFrameFormat::ColorSpace colorSpace() const;
    QVideoFrameFormat::ColorTransfer colorTransfer() const;
    QVideoFrameFormat::ColorRange colorRange() const;
    void setRhi(QRhi* rhi);
    float maxNits();
private:
    QVideoFrameFormat::PixelFormat m_pixelFormat;
    AVFrame *m_frame = nullptr;
    AVFrameUPtr m_hwFrame;
    AVFrameUPtr m_swFrame;
    QSize m_size;
    TextureConverter m_textureConverter;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    std::unique_ptr<TextureSet> m_textures;

};

#endif // FFMPEGVIDEOBUFFER_H
