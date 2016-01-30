#ifndef VECTREXIA_VECTORDRAWING_H
#define VECTREXIA_VECTORDRAWING_H

#include <stdint.h>
#include <vector>
#include <array>
#include <set>
#include <memory>

class Vectorizer
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
        Vector(Vectorizer &vbf);
    };

    struct VectorCompare
    {
        bool operator() (Vector const& l, Vector const& r)
        {

            if (l.x0 < r.x0)
            {
                return true;
            }
            else if (l.x0 == r.x0)
            {
                if (l.x1 < r.x1)
                {
                    return true;
                }
                else if (l.x1 == r.x1)
                {
                    if (l.y0 < r.y0)
                    {
                        return true;
                    }
                    else if (l.y0 == r.y0)
                    {
                        if (l.y1 < r.y1)
                        {
                            return true;
                        }
                        else if (l.y1 == r.y1)
                        {
                            return false;
                        }
                    }
                }
            }
            return false;
        }
    };

    std::set<Vector, VectorCompare> vectors_;
    std::unique_ptr<Vector> current_vector;
    std::array<uint16_t, 135300> framebuffer;

    bool beam_in_range();
    void integrate_axis();
    void center_beam();

public:
    Vectorizer();

    void BeamStep(unsigned char porta, unsigned char portb, unsigned char zero, unsigned char blank);
    const BeamState& getBeamState();

    size_t countVectors() { return vectors_.size(); }

    std::array<uint16_t, 135300> getVectorBuffer();

    void Line(int x0, int y0, int x1, int y1, uint8_t col);
};


#endif //VECTREXIA_VECTORDRAWING_H
