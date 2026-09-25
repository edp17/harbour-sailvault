#include "qrimageprovider.h"

#include <QImage>
#include <QPainter>
#include <QString>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

const int kQrVersion = 4;
const int kQrSize = 33;                 // version * 4 + 17
const int kDataCodewords = 64;          // Version 4 / ECC M
const int kDataCodewordsPerBlock = 32;  // 2 equal blocks
const int kEccCodewordsPerBlock = 18;
const int kTotalCodewords = 100;
const int kQuietZone = 4;
const int kRenderScale = 12;
const int kRenderSize =
    (kQrSize + 2 * kQuietZone) * kRenderScale;

void appendBits(std::vector<bool> &bits,
                std::uint32_t value,
                int length)
{
    for (int i = length - 1; i >= 0; --i)
        bits.push_back(((value >> i) & 1U) != 0);
}

std::uint8_t reedSolomonMultiply(std::uint8_t x,
                                 std::uint8_t y)
{
    int z = 0;

    for (int i = 7; i >= 0; --i) {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }

    return static_cast<std::uint8_t>(z);
}

std::vector<std::uint8_t> reedSolomonDivisor(int degree)
{
    std::vector<std::uint8_t> result(
        static_cast<std::size_t>(degree));

    result.back() = 1;

    std::uint8_t root = 1;

    for (int i = 0; i < degree; ++i) {
        for (std::size_t j = 0; j < result.size(); ++j) {
            result[j] =
                reedSolomonMultiply(result[j], root);

            if (j + 1 < result.size())
                result[j] ^= result[j + 1];
        }

        root = reedSolomonMultiply(root, 0x02);
    }

    return result;
}

std::vector<std::uint8_t> reedSolomonRemainder(
    const std::vector<std::uint8_t> &data,
    const std::vector<std::uint8_t> &divisor)
{
    std::vector<std::uint8_t> result(divisor.size(), 0);

    for (const std::uint8_t byte : data) {
        const std::uint8_t factor =
            byte ^ result.front();

        result.erase(result.begin());
        result.push_back(0);

        for (std::size_t i = 0; i < result.size(); ++i) {
            result[i] ^=
                reedSolomonMultiply(divisor[i], factor);
        }
    }

    return result;
}

class QrMatrix
{
public:
    QrMatrix()
        : m_modules(
              kQrSize,
              std::vector<bool>(kQrSize, false))
        , m_function(
              kQrSize,
              std::vector<bool>(kQrSize, false))
    {
    }

    bool module(int x, int y) const
    {
        return m_modules.at(
            static_cast<std::size_t>(y)).at(
            static_cast<std::size_t>(x));
    }

    void build(const QByteArray &payload)
    {
        if (payload.size() > 62) {
            throw std::length_error(
                "Payload exceeds QR Version 4-M byte capacity");
        }

        const std::vector<std::uint8_t> data =
            createDataCodewords(payload);

        const std::vector<std::uint8_t> all =
            addErrorCorrection(data);

        drawFunctionPatterns();
        drawCodewords(all);

        // Mask pattern 0 is a valid QR mask:
        // (row + column) mod 2 == 0.
        applyMask0();

        // Draw final ECC-M / mask-0 format information.
        drawFormatBits(0);
    }

private:
    void setFunction(int x, int y, bool dark)
    {
        if (x < 0 || x >= kQrSize
                || y < 0 || y >= kQrSize) {
            return;
        }

        m_modules[static_cast<std::size_t>(y)]
                 [static_cast<std::size_t>(x)] = dark;

        m_function[static_cast<std::size_t>(y)]
                  [static_cast<std::size_t>(x)] = true;
    }

    void drawFinder(int centerX, int centerY)
    {
        for (int dy = -4; dy <= 4; ++dy) {
            for (int dx = -4; dx <= 4; ++dx) {
                const int distance =
                    std::max(std::abs(dx), std::abs(dy));

                setFunction(
                    centerX + dx,
                    centerY + dy,
                    distance != 2 && distance != 4);
            }
        }
    }

    void drawAlignment(int centerX, int centerY)
    {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                setFunction(
                    centerX + dx,
                    centerY + dy,
                    std::max(std::abs(dx),
                             std::abs(dy)) != 1);
            }
        }
    }

    static bool bit(int value, int index)
    {
        return ((value >> index) & 1) != 0;
    }

    void drawFormatBits(int mask)
    {
        // QR ECC level M has format value 00.
        const int data = mask;
        int remainder = data;

        for (int i = 0; i < 10; ++i) {
            remainder =
                (remainder << 1)
                ^ ((remainder >> 9) * 0x537);
        }

        const int format =
            ((data << 10) | remainder) ^ 0x5412;

        for (int i = 0; i <= 5; ++i)
            setFunction(8, i, bit(format, i));

        setFunction(8, 7, bit(format, 6));
        setFunction(8, 8, bit(format, 7));
        setFunction(7, 8, bit(format, 8));

        for (int i = 9; i < 15; ++i)
            setFunction(14 - i, 8, bit(format, i));

        for (int i = 0; i < 8; ++i)
            setFunction(
                kQrSize - 1 - i,
                8,
                bit(format, i));

        for (int i = 8; i < 15; ++i)
            setFunction(
                8,
                kQrSize - 15 + i,
                bit(format, i));

        // Fixed dark module.
        setFunction(8, kQrSize - 8, true);
    }

    void drawFunctionPatterns()
    {
        // Timing patterns.
        for (int i = 0; i < kQrSize; ++i) {
            setFunction(6, i, i % 2 == 0);
            setFunction(i, 6, i % 2 == 0);
        }

        // Finder patterns.
        drawFinder(3, 3);
        drawFinder(kQrSize - 4, 3);
        drawFinder(3, kQrSize - 4);

        // Version 4 alignment positions are 6 and 26.
        // Three combinations overlap finder patterns, leaving 26/26.
        drawAlignment(26, 26);

        // Reserve/draw the format-information cells.
        drawFormatBits(0);
    }

    std::vector<std::uint8_t> createDataCodewords(
        const QByteArray &payload) const
    {
        std::vector<bool> bits;

        // Byte mode = 0100.
        appendBits(bits, 0x4, 4);

        // Versions 1-9 use an 8-bit byte-count field.
        appendBits(
            bits,
            static_cast<std::uint32_t>(payload.size()),
            8);

        for (const char c : payload) {
            appendBits(
                bits,
                static_cast<std::uint8_t>(c),
                8);
        }

        const int capacityBits = kDataCodewords * 8;
        const int remaining =
            capacityBits - static_cast<int>(bits.size());

        if (remaining < 0)
            throw std::length_error("QR payload too long");

        appendBits(
            bits,
            0,
            std::min(4, remaining));

        while (bits.size() % 8 != 0)
            bits.push_back(false);

        std::uint8_t pad = 0xEC;

        while (static_cast<int>(bits.size()) < capacityBits) {
            appendBits(bits, pad, 8);
            pad ^= 0xEC ^ 0x11;
        }

        std::vector<std::uint8_t> result(
            kDataCodewords,
            0);

        for (std::size_t i = 0; i < bits.size(); ++i) {
            if (bits[i]) {
                result[i >> 3] |=
                    static_cast<std::uint8_t>(
                        1U << (7 - (i & 7)));
            }
        }

        return result;
    }

    std::vector<std::uint8_t> addErrorCorrection(
        const std::vector<std::uint8_t> &data) const
    {
        if (static_cast<int>(data.size())
                != kDataCodewords) {
            throw std::logic_error(
                "Unexpected QR data-codeword count");
        }

        const std::vector<std::uint8_t> divisor =
            reedSolomonDivisor(kEccCodewordsPerBlock);

        std::vector<std::uint8_t> block0(
            data.begin(),
            data.begin() + kDataCodewordsPerBlock);

        std::vector<std::uint8_t> block1(
            data.begin() + kDataCodewordsPerBlock,
            data.end());

        const std::vector<std::uint8_t> ecc0 =
            reedSolomonRemainder(block0, divisor);

        const std::vector<std::uint8_t> ecc1 =
            reedSolomonRemainder(block1, divisor);

        std::vector<std::uint8_t> result;
        result.reserve(kTotalCodewords);

        // Interleave the two equal data blocks.
        for (int i = 0; i < kDataCodewordsPerBlock; ++i) {
            result.push_back(
                block0[static_cast<std::size_t>(i)]);
            result.push_back(
                block1[static_cast<std::size_t>(i)]);
        }

        // Then interleave ECC codewords.
        for (int i = 0; i < kEccCodewordsPerBlock; ++i) {
            result.push_back(
                ecc0[static_cast<std::size_t>(i)]);
            result.push_back(
                ecc1[static_cast<std::size_t>(i)]);
        }

        return result;
    }

    void drawCodewords(
        const std::vector<std::uint8_t> &codewords)
    {
        std::vector<bool> bits;
        bits.reserve(codewords.size() * 8);

        for (const std::uint8_t byte : codewords) {
            appendBits(bits, byte, 8);
        }

        std::size_t bitIndex = 0;

        for (int right = kQrSize - 1;
             right >= 1;
             right -= 2) {
            if (right == 6)
                right = 5;

            for (int vertical = 0;
                 vertical < kQrSize;
                 ++vertical) {
                for (int j = 0; j < 2; ++j) {
                    const int x = right - j;

                    const bool upward =
                        ((right + 1) & 2) == 0;

                    const int y =
                        upward
                            ? kQrSize - 1 - vertical
                            : vertical;

                    if (!m_function[
                            static_cast<std::size_t>(y)]
                            [static_cast<std::size_t>(x)]
                            && bitIndex < bits.size()) {
                        m_modules[
                            static_cast<std::size_t>(y)]
                            [static_cast<std::size_t>(x)] =
                                bits[bitIndex];

                        ++bitIndex;
                    }
                }
            }
        }

        if (bitIndex != bits.size()) {
            throw std::logic_error(
                "QR codeword placement failed");
        }
    }

    void applyMask0()
    {
        for (int y = 0; y < kQrSize; ++y) {
            for (int x = 0; x < kQrSize; ++x) {
                if (!m_function[
                        static_cast<std::size_t>(y)]
                        [static_cast<std::size_t>(x)]
                        && (x + y) % 2 == 0) {
                    m_modules[
                        static_cast<std::size_t>(y)]
                        [static_cast<std::size_t>(x)] =
                            !m_modules[
                                static_cast<std::size_t>(y)]
                                [static_cast<std::size_t>(x)];
                }
            }
        }
    }

    std::vector<std::vector<bool> > m_modules;
    std::vector<std::vector<bool> > m_function;
};

QImage errorImage()
{
    QImage image(
        kRenderSize,
        kRenderSize,
        QImage::Format_RGB32);

    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    const int margin = kRenderSize / 4;
    const int thickness = kRenderSize / 18;

    painter.drawRect(
        margin,
        kRenderSize / 2 - thickness / 2,
        kRenderSize - 2 * margin,
        thickness);

    painter.end();
    return image;
}

} // namespace

QrImageProvider::QrImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage QrImageProvider::requestImage(
    const QString &id,
    QSize *size,
    const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)

    if (size)
        *size = QSize(kRenderSize, kRenderSize);

    const QByteArray payload = id.toUtf8();

    try {
        QrMatrix qr;
        qr.build(payload);

        QImage image(
            kRenderSize,
            kRenderSize,
            QImage::Format_RGB32);

        image.fill(Qt::white);

        QPainter painter(&image);
        painter.setRenderHint(
            QPainter::Antialiasing,
            false);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);

        for (int y = 0; y < kQrSize; ++y) {
            for (int x = 0; x < kQrSize; ++x) {
                if (!qr.module(x, y))
                    continue;

                painter.drawRect(
                    (x + kQuietZone) * kRenderScale,
                    (y + kQuietZone) * kRenderScale,
                    kRenderScale,
                    kRenderScale);
            }
        }

        painter.end();
        return image;
    } catch (...) {
        return errorImage();
    }
}
