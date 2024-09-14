#ifndef TEXTURECONVERTER_H
#define TEXTURECONVERTER_H

#include <QVideoFrameFormat>
#include <qshareddata.h>
#include <memory>
#include <functional>
#include <mutex>
#include <qt_windows.h>
#include "qffmpeg.h"
#include <rhi/qrhi.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>


using Microsoft::WRL::ComPtr;
template<typename T, typename... Args>
ComPtr<T> makeComObject(Args &&...args)
{
    ComPtr<T> p;
    // Don't use Attach because of MINGW64 bug
    // #892 Microsoft::WRL::ComPtr::Attach leaks references
    *p.GetAddressOf() = new T(std::forward<Args>(args)...);
    return p;
}


class TextureSet {
public:
    // ### Should add QVideoFrameFormat::PixelFormat here
    virtual ~TextureSet() {}
    virtual qint64 textureHandle(QRhi *, int /*plane*/) { return 0; }
};
class D3D11TextureSet : public TextureSet
{
public:
    D3D11TextureSet(QRhi *rhi, ComPtr<ID3D11Texture2D> &&tex)
        : m_owner{ rhi }, m_tex(std::move(tex))
    {
    }

    qint64 textureHandle(QRhi *rhi, int /*plane*/) override
    {
        if (rhi != m_owner)
            return 0u;
        return reinterpret_cast<qint64>(m_tex.Get());
    }

private:
    QRhi *m_owner = nullptr;
    ComPtr<ID3D11Texture2D> m_tex;
};

class TextureConverterBackend
{
public:
    TextureConverterBackend(QRhi *rhi)
        : rhi(rhi)
    {}
    virtual ~TextureConverterBackend() {}
    virtual TextureSet *getTextures(AVFrame * /*frame*/) { return nullptr; }

    QRhi *rhi = nullptr;
};

class TextureConverter
{
    class Data final
    {
    public:
        QAtomicInt ref = 0;
        QRhi *rhi = nullptr;
        AVPixelFormat format = AV_PIX_FMT_NONE;
        std::unique_ptr<TextureConverterBackend> backend;
    };
public:
    TextureConverter(QRhi *rhi = nullptr);

    void init(AVFrame *frame) {
        AVPixelFormat fmt = frame ? AVPixelFormat(frame->format) : AV_PIX_FMT_NONE;
        if (fmt != d->format)
            updateBackend(fmt);
    }
    virtual TextureSet *getTextures(AVFrame *frame);
    bool isNull() const { return !d->backend || !d->backend->rhi; }

private:
    void updateBackend(AVPixelFormat format);

    QExplicitlySharedDataPointer<Data> d;
};


/*! \internal Utility class for synchronized transfer of a texture between two D3D devices
 *
 * This class is used to copy a texture from one device to another device. This
 * is implemented using a shared texture, along with keyed mutexes to synchronize
 * access to the texture.
 *
 * This is needed because we need to copy data from FFmpeg to RHI. FFmpeg and RHI
 * uses different D3D devices.
 */
class TextureBridge final
{
public:
    /** Copy a texture slice at position 'index' belonging to device 'dev'
     * into a shared texture, limiting the texture size to the frame size */
    bool copyToSharedTex(ID3D11Device *dev, ID3D11DeviceContext *ctx,
                         const ComPtr<ID3D11Texture2D> &tex, UINT index, const QSize &frameSize);

    /** Obtain a copy of the texture on a second device 'dev' */
    ComPtr<ID3D11Texture2D> copyFromSharedTex(const ComPtr<ID3D11Device1> &dev,
                                              const ComPtr<ID3D11DeviceContext> &ctx);

private:
    bool ensureDestTex(const ComPtr<ID3D11Device1> &dev);
    bool ensureSrcTex(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex, const QSize &frameSize);
    bool isSrcInitialized(const ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex, const QSize &frameSize) const;
    bool recreateSrc(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex, const QSize &frameSize);

    HANDLE m_sharedHandle{};

    const UINT m_srcKey = 0;
    ComPtr<ID3D11Texture2D> m_srcTex;
    ComPtr<IDXGIKeyedMutex> m_srcMutex;

    const UINT m_destKey = 1;
    ComPtr<ID3D11Device1> m_destDevice;
    ComPtr<ID3D11Texture2D> m_destTex;
    ComPtr<IDXGIKeyedMutex> m_destMutex;

    ComPtr<ID3D11Texture2D> m_outputTex;
};
class D3D11TextureConverter : public TextureConverterBackend
{
public:
    D3D11TextureConverter(QRhi *rhi);

    TextureSet *getTextures(AVFrame *frame) override;

    static void SetupDecoderTextures(AVCodecContext *s);

private:
    ComPtr<ID3D11Device1> m_rhiDevice;
    ComPtr<ID3D11DeviceContext> m_rhiCtx;
    TextureBridge m_bridge;
};

struct TextureDescription
{
    static constexpr int maxPlanes = 3;
    struct SizeScale {
        int x;
        int y;
    };
    using BytesRequired = int(*)(int stride, int height);

    inline int strideForWidth(int width) const { return (width*strideFactor + 15) & ~15; }
    inline int bytesForSize(QSize s) const { return bytesRequired(strideForWidth(s.width()), s.height()); }
    int widthForPlane(int width, int plane) const
    {
        if (plane > nplanes) return 0;
        return (width + sizeScale[plane].x - 1)/sizeScale[plane].x;
    }
    int heightForPlane(int height, int plane) const
    {
        if (plane > nplanes) return 0;
        return (height + sizeScale[plane].y - 1)/sizeScale[plane].y;
    }

    int nplanes;
    int strideFactor;
    BytesRequired bytesRequired;
    QRhiTexture::Format textureFormat[maxPlanes];
    SizeScale sizeScale[maxPlanes];
};

const TextureDescription *textureDescription(QVideoFrameFormat::PixelFormat format);

#endif // TEXTURECONVERTER_H
