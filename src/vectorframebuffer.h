#ifndef VECTREXIA_VECTORDRAWING_H
#define VECTREXIA_VECTORDRAWING_H

#include <stdint.h>
#include <vector>
#include <set>
#include <memory>

class VectorFrameBuffer
{
    const int VECTREX_VECTOR_WIDTH = 33000;
    const int VECTREX_VECTOR_HEIGHT = 41000;

    struct BeamState
    {
        int x, y;
        int rate_x, rate_y;
        bool enabled;

        // internal state
        unsigned int _x_axis, _y_axis, _offset;
        unsigned char _z_axis;
    } beam;

    class Vector
    {
    public:
        int x0, y0, x1, y1;
        int rate_x, rate_y;
        float intensity;
        Vector(VectorFrameBuffer &vbf);
    };

    struct VectorCompare
    {
        bool operator() (std::unique_ptr<Vector> const& v1, std::unique_ptr<Vector> const& v2)
        {
            return v1->x0 < v2->x0 || \
                   v1->y0 < v2->y0 || \
                   v1->x1 < v2->x1 || \
                   v1->y1 < v2->y1 || \
                   v1->intensity < v2->intensity;
        }
    };

    int width_, height_;


    std::set<std::unique_ptr<Vector>, VectorCompare> vectors_;
    std::unique_ptr<Vector> current_vector;

    bool beam_in_range();
    void integrate_axis();
    void center_beam();

public:
    // iterators?

    VectorFrameBuffer();
    uint16_t *GetRawFramebuffer();
    int GetWidth();
    int GetHeight();

    void BeamStep(unsigned char porta, unsigned char portb, unsigned char zero, unsigned char blank);
    const BeamState& getBeamState();

};


#endif //VECTREXIA_VECTORDRAWING_H
