#pragma once

#include<cmath>

using ClipFn = float(*)(float);

float hardClip(float in);
float diodeClip1N4148(float in);