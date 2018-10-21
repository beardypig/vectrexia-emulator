/*
Copyright (C) 2016 beardypig, pelorat

This file is part of Vectrexia.

Vectrexia is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Vectrexia is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Vectrexia.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VECTREXIA_GFXUTIL_H
#define VECTREXIA_GFXUTIL_H

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <type_traits>
#include <array>
#include <memory>
#include <algorithm>
#include <string>
#include "veclib.h"

extern uint8_t font8x8_basic[128][8];

namespace vxgfx
{

/*
 * color channel blending function
 */
inline float blend_channel(const float a, const float b, const float t) {
    return ::sqrt((1.0f - t) * ::pow(a, 2.0f) + t * ::pow(b, 2.0f));
}

/*
 * alpha channel blending function
 */
inline float blend_alpha(const float a, const float b, const float t) {
    return (1.0f - t) * a + t * b;
}

/*
 * ARGB pixel format
 */
struct pf_argb_t {

    using value_type = uint32_t;
    value_type value = static_cast<value_type>(0xff) << 24u;

    inline pf_argb_t() = default;
    inline ~pf_argb_t() = default;
    inline explicit pf_argb_t(value_type v) noexcept : value(v) {}
    inline pf_argb_t(const pf_argb_t&) = default;
    inline pf_argb_t(pf_argb_t&&) = default;
    inline pf_argb_t &operator=(const pf_argb_t&) = default;
    inline pf_argb_t &operator=(pf_argb_t &&) = default;

    constexpr pf_argb_t(uint8_t r, uint8_t g, uint8_t b) noexcept {
        value = static_cast<value_type>(0xff) << 24u
            | static_cast<value_type>(r) << 16u
            | static_cast<value_type>(g) << 8u
            | static_cast<value_type>(b);
    }

    constexpr pf_argb_t(uint8_t a, uint8_t r, uint8_t g, uint8_t b) noexcept {
        value = static_cast<value_type>(a) << 24u
            | static_cast<value_type>(r) << 16u
            | static_cast<value_type>(g) << 8u
            | static_cast<value_type>(b);
    }

    constexpr static uint8_t to_c8(const float &v) {
        return static_cast<uint8_t>(v * 255.0f);
    }

    constexpr static value_type comp_a(const pf_argb_t &c) {
        return (c.value >> 24u) & 0xffu;
    }

    constexpr static value_type comp_r(const pf_argb_t &c) {
        return (c.value >> 16u) & 0xffu;
    }

    constexpr static value_type comp_g(const pf_argb_t &c) {
        return (c.value >> 8u) & 0xffu;
    }

    constexpr static value_type comp_b(const pf_argb_t &c) {
        return c.value & 0xffu;
    }

    constexpr float a() const {
        return static_cast<float>(comp_a(*this)) / 255.0f;
    }

    constexpr float r() const {
        return static_cast<float>(comp_r(*this)) / 255.0f;
    }

    constexpr float g() const {
        return static_cast<float>(comp_g(*this)) / 255.0f;
    }

    constexpr float b() const {
        return static_cast<float>(comp_b(*this)) / 255.0f;
    }

    constexpr void a(uint8_t v) {
        value |= static_cast<value_type>(v) << 24u;
    }

    constexpr void r(uint8_t v) {
        value |= static_cast<value_type>(v) << 16u;
    }

    constexpr void g(uint8_t v) {
        value |= static_cast<value_type>(v) << 8u;
    }

    constexpr void b(uint8_t v) {
        value |= static_cast<value_type>(v);
    }

    constexpr void operator+=(const float v) {
        *this = brightness(v);
    }

    inline pf_argb_t blend(const pf_argb_t &rhs, const float blend_point) const {
        return {
            static_cast<uint8_t>(blend_alpha(a(),   rhs.a(), blend_point) * 255.0f),
            static_cast<uint8_t>(blend_channel(r(), rhs.r(), blend_point) * 255.0f),
            static_cast<uint8_t>(blend_channel(g(), rhs.g(), blend_point) * 255.0f),
            static_cast<uint8_t>(blend_channel(b(), rhs.b(), blend_point) * 255.0f),
        };
    }

    constexpr pf_argb_t brightness(const float v) const {
        auto r_ = to_c8(vxl::clamp(r() + v, 0.0f, 1.0f));
        auto g_ = to_c8(vxl::clamp(g() + v, 0.0f, 1.0f));
        auto b_ = to_c8(vxl::clamp(b() + v, 0.0f, 1.0f));
        return { r_, g_, b_ };
    }
};

struct pf_rgb565_t {
    using value_type = uint16_t;
    value_type value = 0;
    
    constexpr pf_rgb565_t() = default;

    constexpr static value_type comp_r(const pf_rgb565_t &c) {
        return static_cast<value_type>(c.value >> 11u & 0x1fu);
    }

    constexpr static value_type comp_g(const pf_rgb565_t &c) {
        return static_cast<value_type>(c.value >> 5u & 0x3fu);
    }

    constexpr static value_type comp_b(const pf_rgb565_t &c) {
        return static_cast<value_type>(c.value & 0x1fu);
    }

    constexpr float r() const {
        return comp_r(*this) / 31.0f;
    }

    constexpr float g() const {
        return comp_g(*this) / 63.0f;
    }

    constexpr float b() const {
        return comp_b(*this) / 31.0f;
    }

    constexpr static uint8_t to_c8(const float &v) {
        return static_cast<uint8_t>(v * 255.0f);
    }

    inline pf_rgb565_t brightness(const float v) const {
        auto r_ = to_c8(vxl::clamp(r() + v, 0.0f, 1.0f));
        auto g_ = to_c8(vxl::clamp(g() + v, 0.0f, 1.0f));
        auto b_ = to_c8(vxl::clamp(b() + v, 0.0f, 1.0f));
        return { r_, g_, b_ };
    }

    inline explicit pf_rgb565_t(const pf_argb_t v)
        : pf_rgb565_t(
            static_cast<uint8_t>(v.r() * v.a()),
            static_cast<uint8_t>(v.g() * v.a()),
            static_cast<uint8_t>(v.b() * v.a()))
    { /* ... */ }

    constexpr pf_rgb565_t(uint8_t r, uint8_t g, uint8_t b) {
        value = static_cast<uint16_t>(
            (r >> 3u & 0x1fu) << 11u |
            (g >> 2u & 0x3fu) << 5u |
            (b >> 3u & 0x1fu));
    }
};

/*
 * Monochrome luminosity based pixel format
 */
struct pf_mono_t {
    using value_type = float;
    value_type value = 0.0f;

    pf_mono_t() = default;
    ~pf_mono_t() = default;
    inline pf_mono_t(const pf_mono_t&) = default;
    inline pf_mono_t(pf_mono_t&&) = default;
    inline pf_mono_t &operator=(const pf_mono_t&) = default;
    inline pf_mono_t &operator=(pf_mono_t &&) = default;

    constexpr explicit pf_mono_t(value_type v) noexcept : value(v) {}

    //
    // This constructor performs color to grayscale conversion. The alpha value will
    // darken or brighten the image since this pixel format has no alpha support.
    constexpr explicit pf_mono_t(pf_argb_t argb) noexcept {
        value = (1.0f / 0xff) * argb.a() * (0.2627f * argb.r() + 0.6780f * argb.g() + 0.0593f * argb.b());
    }

    //
    // This constructor performs color to grayscale conversion of the three RGB arguments
    constexpr pf_mono_t(uint8_t r, uint8_t g, uint8_t b) noexcept {
        value = 0.2627f * r + 0.6780f * g + 0.0593f * b;
    }

    constexpr float a() const {
        return value;
    }

    constexpr float r() const {
        return value;
    }

    constexpr float g() const {
        return value;
    }

    constexpr float b() const {
        return value;
    }

    constexpr void operator+= (const float &v) {
        value += v;
    }

    constexpr void operator+= (const pf_mono_t &v) {
        value += v.value;
    }

    inline pf_mono_t blend(const pf_mono_t &rhs, const float blend_point) const {
        return pf_mono_t{ (value * blend_point) + rhs.value * (1.0f - blend_point) };
    }

    constexpr void blend(const pf_mono_t &rhs, const float blend_point) {
        value = (value * blend_point) + rhs.value * (1.0f - blend_point);
    }

    inline pf_mono_t brightness(const pf_mono_t &v) const {
        return pf_mono_t{ value + v.value };
    }

    inline pf_mono_t operator* (float v) const {
        return pf_mono_t{ value * v };
    }
};

/*
 * Line drawing mode: direct (overwrite)
 */
struct m_direct {
    template<typename Fb, typename Pf = decltype(Fb::value_type)>
    constexpr void operator()(Fb &fb, size_t pos, const Pf &color) const {
        fb.data()[pos] = color;
    }
};

/*
 * Line drawing mode: brightness (additive)
 */
struct m_brightness {
    template<typename Fb, typename Pf = decltype(Fb::value_type)>
    constexpr void operator()(Fb &fb, size_t pos, const Pf &color) const {
        fb.data()[pos] += color;
    }
};

/*
 * Line drawing mode: colour blending
 */
template <int Bp = 50>
struct m_blend {
    template<typename Fb, typename Pf = decltype(Fb::value_type)>
    constexpr void operator()(Fb &fb, size_t pos, const Pf &color) const {
        fb.data()[pos].blend(color, (Bp / 100.0f));
    }
};

struct point_t {
    constexpr point_t(int x_, int y_) : x(x_), y(y_) {}
    int x;
    int y;
};

struct rect_t
{
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    constexpr rect_t() = default;

    constexpr rect_t(int x, int y, int w, int h)
        : left(x), top(y), right(x + w), bottom(y + h)
    { /* ... */
    }

    constexpr rect_t(int w, int h)
        : right(w), bottom(h)
    { /* ... */
    }

    constexpr rect_t(const point_t tl, const point_t br)
        : left(tl.x), top(tl.y), right(br.x), bottom(br.y)
    { /* ... */
    }

    constexpr rect_t(const point_t *tl, const point_t *br)
        : left(tl->x), top(tl->y), right(br->x), bottom(br->y)
    { /* ... */
    }

    constexpr int area() const {
        return width() * height();
    }

    constexpr operator bool() const {
        return area() > 0;
    }

    constexpr int width() const {
        return right - left;
    }

    constexpr int height() const {
        return bottom - top;
    }

    constexpr void offset(const int x, const int y) {
        left += x;
        right += x;
        top += y;
        bottom += y;
    }

    constexpr void move(const point_t &p) {
        return move(p.x, p.y);
    }

    constexpr void move(const int x, const int y) {
        offset(x - left, y - top);
    }

    constexpr void normalize() {
        if (left > right) {
            std::swap(left, right);
        }
        if (top > bottom) {
            std::swap(top, bottom);
        }
    }
};

/*
 * Framebuffer class, thin wrapper for an array in a unique_ptr
 *
 * Usage:
 *    vxgfx::framebuffer<WIDTH, HEIGHT, PIXEL_FORMAT> buffer;
 *
 */
template<size_t W, size_t H, typename Pf>
class framebuffer
{
private:
    using data_type = std::array<Pf, W*H>;
public:

    //
    // define some types that can referenced by others

    using value_type = Pf;
    using pointer = value_type * ;
    using reference = value_type & ;
    using iterator = typename data_type::iterator;
    using const_iterator = typename data_type::const_iterator;

    const int width = W;
    const int height = H;

    //
    // STL compatible iterator pass-throughs

    constexpr auto begin()->iterator {
        return buffer->begin();
    }

    constexpr auto begin() const ->const_iterator {
        return buffer->cbegin();
    }

    constexpr auto cbegin() const ->const_iterator {
        return buffer->cbegin();
    }

    constexpr auto end()->iterator {
        return buffer->end();
    }

    constexpr auto end() const ->const_iterator {
        return buffer->end();
    }

    constexpr auto cend() const ->const_iterator {
        return buffer->cend();
    }

    constexpr framebuffer() {
        buffer = std::make_unique<data_type>();
    }

    
    // Clears the internal array<> using the pixel format default (Pf)
    constexpr void clear() {
        buffer->fill(Pf{});
    }

    // Fill buffer with colour
    constexpr void fill(Pf c) {
        buffer->fill(std::move(c));
    }

    // Returns the array size
    constexpr size_t size() const {
        return buffer->size();
    }

    // Returns a pointer to the internal array<>
    constexpr pointer data() const {
        return buffer.get()->data();
    }

    const rect_t rect() const {
        return rect_t(W, H);
    }

    //
    // copy / constructors / operators

    constexpr explicit framebuffer(Pf c)
    : framebuffer() {
        fill(std::move(c));
    }

    constexpr framebuffer(const framebuffer &rhs)
        : framebuffer() {
        *this = rhs;
    }

    constexpr framebuffer &operator=(const framebuffer &rhs) {
        *buffer = *rhs.buffer;
        return *this;
    }

    constexpr framebuffer(framebuffer &&rhs) {
        *this = std::move(rhs);
    }

    constexpr framebuffer &operator=(framebuffer &&rhs) {
        buffer = std::move(rhs.buffer);
        return *this;
    }

    template<typename DrawMode>
    constexpr void plot_pixel(const int x, const int y, DrawMode mode, Pf color) {
        if (x < width && x >= 0 && y < height && y >= 0) {
            mode(*this, (y * width) + x, color);
        }
    }

    const Pf get_pixel(const int x, const int y) const {
        return (x < width && x >= 0 && y < height && y >= 0)
            ? (*buffer.get())[(y * width) + x] : Pf();
    }

    ~framebuffer() = default;

private:
    std::unique_ptr<data_type> buffer{};
};

/*
 * vectrex viewport voltage span
 */
struct viewport {
    float l = -2.5f;
    float r = +2.5f;
    float t = -5.0f;
    float b = +5.0f;
    using pointer = viewport * ;
    viewport() = default;
    viewport(float width, float height) :
        l(-width / 2), t(-height / 2),
        r(width / 2), b(height / 2) {}
    void offset(float x, float y) {
        l += x; r += x;
        t += y; b += y;
    }
    auto translate(float x, float y, int w, int h)
        ->std::pair<int, int> {
        return std::make_pair(
            static_cast<int>((x - l) / (r - l) * w),
            static_cast<int>((y - t) / (b - t) * h));
    }
};

/*
 * Basic line drawing function
 */
template<typename DrawMode, typename T, typename Pf = decltype(T::value_type)>
void draw_line(T &fb, int x0, int y0, int x1, int y1, const Pf &c)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (1)
    {
        fb.plot_pixel(x0, y0, DrawMode(), c);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err = err - dy;
            x0 = x0 + sx;
        }

        if (e2 < dx)
        {
            err = err + dx;
            y0 = y0 + sy;
        }
    }
}

/*
 * Antialiased line drawing. No endpoint calculations due to int coordinates.
 * https://en.wikipedia.org/wiki/Xiaolin_Wu's_line_algorithm
 *
 * TODO: Optimization for horizontal and vertical lines. Currently, no matter
 * the drawing direction, it will always call plot_pixel() twice even when not
 * needed, for instance when the brightness for one of them is 0.0f which is
 * the case for vertical and horizontal lines.
 */
template<typename DrawMode, typename T, typename Pf = decltype(T::value_type)>
void draw_aline(T &fb, int x0, int y0, int x1, int y1, const Pf &c)
{
    auto itrunc  = [](float v) ->int { return static_cast<int>(v); };
    auto ftrunc  = [](float v) -> float { return std::floor(v); };
    auto rftrunc = [](float v) -> float { return 1.0f - std::floor(v); };

    auto steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    auto iy = static_cast<float>(y0);
    auto dx = static_cast<float>(x1) - x0;
    auto dy = static_cast<float>(y1) - y0;
    auto gradient = (dx == 0.0f) ? 1.0f : dy / dx;
    
    if (steep)
    {
        for (int x = x0; x <= x1; ++x)
        {
            Pf p1 = c * rftrunc(iy);
            Pf p2 = c * ftrunc(iy);
            fb.plot_pixel(itrunc(iy), x, DrawMode(), p1);
            fb.plot_pixel(itrunc(iy) - 1, x, DrawMode(), p2);
            iy += gradient;
        }
    }
    else
    {
        for (int x = x0; x <= x1; ++x)
        {
            Pf p1 = c * rftrunc(iy);
            Pf p2 = c * ftrunc(iy);
            fb.plot_pixel(x, itrunc(iy), DrawMode(), p1);
            fb.plot_pixel(x, itrunc(iy) - 1, DrawMode(), p2);
            iy += gradient;
        }
    }
}

/*
 * Voltage based line drawing
 */
template<typename DrawMode, typename T, typename Pf = decltype(T::value_type)>
void draw_line(T &fb, viewport &vp, float x0, float y0, float x1, float y1, const Pf &c) {
    auto p1 = vp.translate(x0, y0, fb.width, fb.height);
    auto p2 = vp.translate(x1, y1, fb.width, fb.height);
    draw_line<DrawMode>(fb, p1.first, p1.second, p2.first, p2.second, c);
}

/*
 * Text drawing
 */
constexpr unsigned int PIXEL_WIDTH = 8;
constexpr unsigned int PIXEL_HEIGHT = 8;
constexpr unsigned int PIXEL_SPACING = 1;

template<typename DrawMode, typename T, typename Pf = decltype(T::value_type)>
void draw_text(T &fb, int x, int y, const Pf &c, std::string message) {
    for (auto &m : message) {
        // each character is 8 x 8 pixels
        for (auto y_pixel = 0; y_pixel < PIXEL_HEIGHT; y_pixel++) {
            for (uint8_t x_pixel = 0; x_pixel < PIXEL_WIDTH; x_pixel++) {
                auto fchar = font8x8_basic[m & 0x7fu][y_pixel];
                if (fchar & (1u << x_pixel)) {  // draw the pixel or not
                    fb.plot_pixel(x + x_pixel, y + y_pixel, DrawMode(), c);
                }
            }
        }
        x += PIXEL_WIDTH + PIXEL_SPACING;
    }
}

inline rect_t intersect(const rect_t *a, const rect_t *b) {

    const point_t p0{
        std::max(a->left, b->left),
        std::max(a->top, b->top)
    };

    const point_t p1{
        std::min(a->right, b->right),
        std::min(a->bottom, b->bottom)
    };

    return (p0.x >= p1.x || p0.y >= p1.y)
        ? rect_t{} : rect_t{ p0, p1 };
}

inline rect_t intersect(const rect_t &a, const rect_t &b) {
    return intersect(&a, &b);
}

struct transform {
    rect_t src;
    rect_t dst;
    float wr;       // ratio width
    float hr;       // ratio height

    transform(const rect_t s, const rect_t d)
    : src(std::move(s)), dst(std::move(d)) {
        wr = static_cast<float>(s.width()) / d.width();
        hr = static_cast<float>(s.height()) / d.height();
    }
    
    point_t translate(const int dx, const int dy) const {
        return point_t(
            static_cast<int>(dx * wr),
            static_cast<int>(dy * hr)
        );
    }

    point_t translate(const point_t p) {
        return translate(p.x, p.y);
    }
};

template<typename PfDst, typename PfSrc, typename Fn>
void draw(PfDst &pfDst, point_t offset, PfSrc &pfSrc, const rect_t &passepartout, Fn draw) {

    // Initialized in all branches so not needed here
    int px;
    int py;
    int pw;
    int ph;

    // Get framebuffer rects
    const auto srcRect = pfSrc.rect();
    auto dstRect = pfDst.rect();

    if (srcRect == dstRect && offset.x == 0 && offset.y == 0)
    {   
        // This is a 1:1 overlay, so take a shortcut
        px = 0;             // cutout x
        py = 0;             // cutout y
        pw = pfSrc.width;     // cutout w
        ph = pfSrc.height;    // cutout h
    }
    else
    {
        // Calcuate the source cutout rectangle, and if it's empty
        // we do nothing, just exit the function.
        auto ppRect = (passepartout == srcRect)
            ? srcRect : intersect(srcRect, passepartout);

        if (!ppRect)
            return;

        // Create a rect for calculating the intersection with the
        // destination framebuffer. This is the cutout adjusted to
        // the offset. We also create a copy so we can record how
        // the rectangle changes (i.e. which edges are moved).
        rect_t dstIntersect(offset.x, offset.y, ppRect.width(), ppRect.height());
        rect_t dstIntersectCopy = dstIntersect;

        // calculate the intersection and exit if empty
        dstRect = intersect(dstRect, dstIntersect);
        if (!dstRect)
            return;

        // Calculate how the intersection changed relative to the copy
        // this is how we will need to adjust the original cutout, then
        // adjust the original cutout rect. This is the absolute source
        // rect we need to copy to the destination buffer. If the math
        // has worked out it's entierly contained in the bounds of srcRect.
        ppRect.left += (dstRect.left - dstIntersectCopy.left);
        ppRect.top += (dstRect.top - dstIntersectCopy.top);
        ppRect.right += (dstRect.right - dstIntersectCopy.right);
        ppRect.bottom += (dstRect.bottom - dstIntersectCopy.bottom);

        px = ppRect.left;
        py = ppRect.top;
        pw = ppRect.width();
        ph = ppRect.height();
    }

    auto rawDst = pfDst.data();
    auto rawSrc = pfSrc.data();

    // Copy loop.
    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            auto srcPos = ((y + py) * pfSrc.width) + (x + px);
            auto dstPos = ((y + offset.y) * pfDst.width) + (x + offset.x);
            draw(rawDst[dstPos], rawSrc[srcPos]);
        }
    }
}

} // namespace vxgfx

#endif //VECTREXIA_GFXUTIL_H
