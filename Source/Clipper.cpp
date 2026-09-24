#include "Clipper.h"

float hardClip(float in)
{
    return (fabs(in) > 1.0) ? (in > 0 ? 1.0f : -1.0f) : in;
}