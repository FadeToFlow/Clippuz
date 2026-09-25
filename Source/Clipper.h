#pragma once

#include<cmath>

using ClipFn = float(*)(float);

float hardClip(float in);
float diodeClip1N4148(float in);
float diodeClipDO7(float in);
float diodeClipDO7cubic(float in);

float quadClip(float in);
float quadClipFold(float in);