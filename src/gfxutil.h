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

    const size_t width = W;
    const size_t height = H;

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
    auto translate(float x, float y, size_t w, size_t h)
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
        auto pos = (y0 * fb.width) + x0;
        if (x0 < fb.width && x0 >= 0 && y0 < fb.height && y0 >= 0) {
            DrawMode()(fb, pos, c);
        }

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
                    auto pos = ((y + y_pixel) * fb.width) + x + x_pixel;
                    DrawMode()(fb, pos, c);
                }
            }
        }
        x += PIXEL_WIDTH + PIXEL_SPACING;
    }
}

struct point_t {
    int32_t  x;
    int32_t  y;
};

struct rect_t
{
    int32_t    left = 0;
    int32_t    top = 0;
    int32_t    right = 0;
    int32_t    bottom = 0;

    constexpr rect_t() = default;

    constexpr rect_t(int32_t x, int32_t y, int32_t w, int32_t h)
        : left(x), top(y), right(x + w), bottom(y + h)
    { /* ... */
    }

    constexpr rect_t(int32_t w, int32_t h)
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

    constexpr int32_t area() const {
        return width() * height();
    }

    constexpr operator bool() const {
        return area() > 0;
    }

    constexpr int32_t width() const {
        return right - left;
    }

    constexpr int32_t height() const {
        return bottom - top;
    }

    constexpr void offset(const int32_t x, const int32_t y) {
        left += x;
        right += x;
        top += y;
        bottom += y;
    }

    constexpr void move(const point_t &p) {
        return move(p.x, p.y);
    }

    constexpr void move(const int32_t x, const int32_t y) {
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

template<typename Dst, typename Src, typename Fn>
void draw(Dst &dst, Src &src, Fn draw_) {
  draw(dst, point_t{ 0, 0 }, src, rect_t{static_cast<uint32_t>(src.width), static_cast<uint32_t>(src.height)}, draw_);
}

template<typename Dst, typename Src, typename Fn>
void draw(Dst &dst, point_t offset, Src &src, const rect_t &passepartout, Fn draw) {

    // Rectangle representing the entire source framebuffer
    rect_t srcRect{
        static_cast<int32_t>(src.width),
        static_cast<int32_t>(src.height)
    };

    // Rectangle representing the entire destination framebuffer
    const rect_t dstRect{
        static_cast<int32_t>(dst.width),
        static_cast<int32_t>(dst.height)
    };

    // Calcuate the source cutout rectangle, and if it's empty
    // we do nothing, just exit the function.
    auto ppRect = intersect(srcRect, passepartout);
    if (!ppRect)
        return;

    // Create a rect for calculating the intersection with the
    // destination framebuffer. This is the cutout adjusted to
    // the offset. We also create a copy so we can record how
    // the rectangle changes (i.e. which edges are moved).
    rect_t dstIntersect(offset.x, offset.y, ppRect.width(), ppRect.height());
    rect_t dstIntersectCopy = dstIntersect;

    // calculate the intersection and exit if empty
    dstIntersect = intersect(dstRect, dstIntersect);
    if (!dstIntersect)
        return;

    // Calculate how the intersection changed relative to the copy
    // this is how we will need to adjust the original cutout
    int ld = dstIntersect.left - dstIntersectCopy.left;
    int td = dstIntersect.top - dstIntersectCopy.top;
    int rd = dstIntersect.right - dstIntersectCopy.right;
    int bd = dstIntersect.bottom - dstIntersectCopy.bottom;

    // Adjust the original cutout rect. This is the absolute source
    // rect we need to copy to the destination buffer. If the math
    // has worked out it's entierly contained in the bounds of srcRect.
    ppRect.left += ld;
    ppRect.top += td;
    ppRect.right += rd;
    ppRect.bottom += bd;

    auto rawDst = dst.data();
    auto rawSrc = src.data();

    // Copy loop.
    for (int32_t y = 0; y < ppRect.height(); y++) {
        for (int32_t x = 0; x < ppRect.width(); x++) {
            auto srcPos = ((y + ppRect.top) * src.width) + (x + ppRect.left);
            auto dstPos = ((y + offset.y) * dst.width) + (x + offset.x);
            draw(rawDst[dstPos], rawSrc[srcPos]);
        }
    }
}

} // namespace vxgfx

/*
struct color_t
{
    float r, g, b;
    float intensity; // clamped to 0.0f, 1.0f
    color_t operator+(const color_t&color) const
    {
        return color_t{color.intensity * color.r + (1 - color.intensity) * r,
                       color.intensity * color.g + (1 - color.intensity) * g,
                       color.intensity * color.b + (1 - color.intensity) * b,
                       1.0f}; // intensity is now 1.0f
    }
    color_t operator-(const float &intensity) const
    {
        return color_t{(1.0f - intensity) * r,
                       (1.0f - intensity) * g,
                       (1.0f - intensity) * b,
                       1.0f};
    }
    color_t& operator+=(const color_t& color)
    {
        r = color.intensity * color.r + (1 - color.intensity) * r,
        g = color.intensity * color.g + (1 - color.intensity) * g,
        b = color.intensity * color.b + (1 - color.intensity) * b,
        intensity = 1.0f;
        return *this;
    }
    color_t& operator-=(const float& intensity)
    {
        r = (1.0f - intensity) * r;
        g = (1.0f - intensity) * g;
        b = (1.0f - intensity) * b;
        this->intensity = 1.0f;
        return *this;
    }
    color_t& operator=(const color_t& color)
    {
        r = color.r;
        g = color.g;
        b = color.b;
        intensity = color.intensity;
        return *this;
    }
    uint16_t rgb565() const
    {
        uint8_t ri = (uint8_t)(r*0xff);
        uint8_t gi = (uint8_t)(g*0xff);
        uint8_t bi = (uint8_t)(b*0xff);
        return (uint16_t) (((ri >> 3) & 0x1f) << 11 | ((gi >> 2) & 0x3f) << 5 | ((bi >> 3) & 0x1f));
    }
    uint32_t rgba8() const
    {
        uint8_t ri = (uint8_t)(r*0xff);
        uint8_t gi = (uint8_t)(g*0xff);
        uint8_t bi = (uint8_t)(b*0xff);
        return (uint32_t) (ri << 16) | (gi << 8) | (bi);
    }

};
 
template <int width, int height>
class Framebuffer
{

    float min_v, max_v;
    std::array<color_t, width*height> framebuffer_;

    inline float normalise(float x, float min, float max)
    {
        return (x - min) / (max - min);
    }

public:
    Framebuffer(float min_v, float max_v)
    {
        this->min_v = min_v;
        this->max_v = max_v;
    }

    Framebuffer<width, height> operator+(const Framebuffer<width, height> &rhs)
    {
        Framebuffer<width, height> fb(min_v, max_v);
        for (int i = 0; i < width * height; i++)
        {
            fb.framebuffer_[i] = framebuffer_[i] + rhs.framebuffer_[i];
        }
        return fb;
    }

    void operator+=(const Framebuffer<width, height> &rhs)
    {
        for (int i; i < width * height; i++)
        {
            framebuffer_[i] += rhs.framebuffer_[i];
        }
    }

    void draw_line(int x0, int y0, int x1, int y1, float intensity)
    {
        draw_line(x0, y0, x1, y1, color_t{1.0f, 1.0f, 1.0f, intensity});
    }

    void draw_line(int x0, int y0, int x1, int y1, color_t color)
    {
        int dx = abs(x1-x0);
        int dy = abs(y1-y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;
        int e2;

        while(1)
        {
            auto pos = (y0 * width) + x0;
            if (x0 < width && x0 >= 0 && y0 < height && y0 >= 0)
                framebuffer_[pos] += color;

            if (x0 == x1 && y0 == y1)
                break;

            e2 = 2 * err;
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

    void draw_debug_text(int x, int y, color_t color, std::string message)
    {
        for (const char &c: message)
        {
            for (int fy=0; fy < 8; fy++) {
                for (int fx=0; fx < 8; fx++) {
                    auto fchar = font8x8_basic[c & 0x7f][fy];
                    if (fchar & (1 << fx))
                        framebuffer_[((y + fy) * width) + x + fx] += color;
                }
            }
            x += 8 + 1; // + 1 for spacing
        }
    }

    void draw_debug_text(int x, int y, color_t color, const char* fmt, ...)
    {
        va_list args;

        va_start(args, fmt);
        vdraw_debug_text(x, y, color, fmt, args);
        va_end(args);
    }

    void vdraw_debug_text(int x, int y, color_t color, const char* fmt, va_list args)
    {
        char buf[2000];
        vsnprintf(buf, 2000, fmt, args);
        draw_debug_text(x, y, color, std::string(buf));
    }

    void draw_line(float fx0, float fy0, float fx1, float fy1, float intensity)
    {
        draw_line(fx0, fy0, fx1, fy1, color_t{1.0f, 1.0f, 1.0f, intensity});
    }

    void draw_line(float fx0, float fy0, float fx1, float fy1, color_t color)
    {
        int x0 = (int) (normalise(fx0, min_v / 2, max_v / 2) * (float) width);
        int y0 = (int) (normalise(fy0, min_v, max_v) * (float) height);
        int x1 = (int) (normalise(fx1, min_v / 2, max_v / 2) * (float) width);
        int y1 = (int) (normalise(fy1, min_v, max_v) * (float) height);
        draw_line(x0, y0, x1, y1, color);
    }

    // draw grid at voltage inteval
    void draw_debug_grid(color_t line, float xi, float yi)
    {
        // left + right
        draw_line(min_v / 2, min_v, min_v / 2, max_v, line);
        draw_line(max_v / 2, min_v, max_v / 2, max_v, line);

        for (float x = (min_v / 2) + xi; x < max_v / 2.0f; x += xi)
        {
            draw_line(x, min_v, x, max_v, line);
        }

        // top + bottom
        draw_line(min_v / 2, min_v, max_v / 2, min_v, line);
        draw_line(min_v / 2, max_v, max_v / 2, max_v, line);

        for (float y = min_v + yi; y < max_v; y += yi)
        {
            draw_line(min_v / 2.0f, y, max_v / 2.0f, y, line);
        }
    }

    void fill(color_t color)
    {
        framebuffer_.fill(color);
    }

    void fill()
    {
        fill({0.0f, 0.0f, 0.0f, 0.0f});
    }

    std::array<uint16_t, width * height> rgb565() const
    {
        std::array<uint16_t, width * height> fb;
        for (int i = 0; i < width * height; i++)
        {
            fb[i] = framebuffer_[i].rgb565();
        }
        return fb;
    };

    std::array<uint32_t, width * height> rgb8() const
    {
        std::array<uint32_t, width * height> fb;
        for (int i = 0; i < width * height; i++)
        {
            fb[i] = framebuffer_[i].rgb565();
        }
        return fb;
    };

    vxgfx::framebuffer<10, 10, uint32_t> test{};
};
*/

#endif //VECTREXIA_GFXUTIL_H
