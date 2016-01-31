#ifndef VECTREXIA_VECTORDRAWING_H
#define VECTREXIA_VECTORDRAWING_H

#include <stdint.h>
#include <vector>
#include <array>
#include <set>
#include <memory>

class Vectorizer
{
    const int VECTOR_WIDTH = 33000;
    const int VECTOR_HEIGHT = 41000;

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
        uint64_t start_cycle;
        uint64_t end_cycle;
        Vector(Vectorizer &vbf);
    };

    std::unique_ptr<Vector> current_vector;
    std::array<float, 135300> framebuffer;
    uint64_t cycles = 0;
    bool beam_in_range();

    void integrate_axis();
    void center_beam();
    void draw_line(int x0, int y0, int x1, int y1, float starti, float endi);
public:

    std::vector<Vector> vectors_;

    Vectorizer();
    void BeamStep(unsigned char porta, unsigned char portb, unsigned char zero, unsigned char blank);
    const BeamState& getBeamState();
    size_t countVectors() { return vectors_.size(); }
    std::array<float, 135300> getVectorBuffer();
};


#endif //VECTREXIA_VECTORDRAWING_H
