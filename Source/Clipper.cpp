#include "Clipper.h"



float hardClip(float in)
{
    return (fabs(in) > 1.0) ? (in > 0 ? 1.0f : -1.0f) : in;
}

static constexpr int    N      = 32;
static constexpr double U_MAX  = 4.0;
static constexpr double STEP   = U_MAX / (N - 1);          // 4/31

static const float diode1N4148[N] = {
    0.0000000000,
    0.1290321369,
    0.2580593907,
    0.3868868307,
    0.5091193895,
    0.5839780386,
    0.6169272292,
    0.6356711113,
    0.6484440224,
    0.6577012320,
    0.6651800612,
    0.6719038648,
    0.6769322253,
    0.6818512555,
    0.6860650698,
    0.6896580515,
    0.6932510333,
    0.6966440458,
    0.6992694821,
    0.7018949184,
    0.7045203547,
    0.7071457910,
    0.7093492439,
    0.7112722966,
    0.7131953493,
    0.7151184019,
    0.7170414546,
    0.7189645073,
    0.7207823213,
    0.7221973634,
    0.7236124056,
    0.7250274477,
};

static const float diodeDO7[N] = {
    0.0000000000,
    0.1009791625,
    0.1675443583,
    0.2110715832,
    0.2427525506,
    0.2679296487,
    0.2891405622,
    0.3077151462,
    0.3244606306,
    0.3397844517,
    0.3540990686,
    0.3676053164,
    0.3804316471,
    0.3926916922,
    0.4044854518,
    0.4159004779,
    0.4270130273,
    0.4378048328,
    0.4483029732,
    0.4586557855,
    0.4687257473,
    0.4786664901,
    0.4884238552,
    0.4980484575,
    0.5075217798,
    0.5169125020,
    0.5261278481,
    0.5353431943,
    0.5443359614,
    0.5533171058,
    0.5622271858,
    0.5709957941,
};

float diodeClip1N4148(float in) 
{
    float amp = diode1N4148[N-1];
    float absIn = fabs(in) * amp;
    float signIn = in >= 0 ? 1.0f : -1.0f;

    if (absIn >= U_MAX) {
        return diode1N4148[N-1] * signIn / amp;
    }

    const float pos = absIn / STEP;     
    int i = static_cast<int>(pos);         
    if (i > N - 2) i = N - 2;           

    const float t  = pos - i;            
    const float y0 = diode1N4148[i];
    const float y1 = diode1N4148[i + 1];

    return (y0 + t * (y1 - y0)) * signIn / amp;
}

float diodeClipDO7(float in) 
{
    float amp = diodeDO7[N-1];
    float absIn = fabs(in) * amp;
    float signIn = in >= 0 ? 1.0f : -1.0f;

    if (absIn >= U_MAX) {
        return diodeDO7[N-1] * signIn / amp;
    }

    const float pos = absIn / STEP;     
    int i = static_cast<int>(pos);         
    if (i > N - 2) i = N - 2;           

    const float t  = pos - i;            
    const float y0 = diodeDO7[i];
    const float y1 = diodeDO7[i + 1];

    return (y0 + t * (y1 - y0)) * signIn / amp;
}