/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <math.h>

#include <swamp-runtime/core/bind.h>
#include <swamp-runtime/core/math.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/swamp.h>
#include <tiny-libc/tiny_libc.h>

#define FIXED_MULTIPLIER (1000)
#define MAX_FIXED_RADIAN (6280)
#define MAX_DEGREE (360)


static int fastAtan2(signed int x, signed int y)
{
    unsigned char negative_flags;
    unsigned char temp_degree;
    unsigned char degree_compensation;
    unsigned int degree;
    unsigned int ux;
    unsigned int uy;

    negative_flags = 0;
    if (x < 0) {
        negative_flags += 0x01;
        x = (0 - x);
    }
    ux = x;
    if (y < 0) {
        negative_flags += 0x02;
        y = (0 - y);
    }
    uy = y;

    if (ux > uy) {
        degree = (uy * 45) / ux;
        negative_flags += 0x10;
    } else {
        degree = (ux * 45) / uy;
    }

    degree_compensation = 0;
    temp_degree = degree;
    if (temp_degree > 22) {
        if (temp_degree <= 44)
            degree_compensation++;
        if (temp_degree <= 41)
            degree_compensation++;
        if (temp_degree <= 37)
            degree_compensation++;
        if (temp_degree <= 32)
            degree_compensation++;
    } else {
        if (temp_degree >= 2)
            degree_compensation++;
        if (temp_degree >= 6)
            degree_compensation++;
        if (temp_degree >= 10)
            degree_compensation++;
        if (temp_degree >= 15)
            degree_compensation++;
    }
    degree += degree_compensation;

    if (negative_flags & 0x10)
        degree = (90 - degree);

    if (negative_flags & 0x02) {
        if (negative_flags & 0x01)
            degree = (180 + degree);
        else
            degree = (180 - degree);
    } else {
        if (negative_flags & 0x01)
            degree = (360 - degree);
    }

    return degree;
}

static unsigned int pseudoRandom(unsigned int start, unsigned int modulus)
{
    unsigned int value = (214013 * start + 2531011);
    value = (value >> 16) & 0x7FFF;
    return value % modulus;
}

#define SWAMP_CORE_SIN_TABLE_MAX (256)
static int g_sin_table[SWAMP_CORE_SIN_TABLE_MAX];
static int g_sin_table_initialized = 0;
static int sin_multiplier = 1000;
static const float SWAMP_CORE_MATH_TWO_PI = 6.2831f;
static const SwampFixed32 SWAMP_CORE_MATH_TWO_PI_FIXED = MAX_FIXED_RADIAN;

static void init_sin_table(int sine[])
{
    for (int i = 0; i < SWAMP_CORE_SIN_TABLE_MAX; ++i) {
        double angle = (SWAMP_CORE_MATH_TWO_PI * (double) i) / (double) SWAMP_CORE_SIN_TABLE_MAX;
        float sin_value = sinf(angle);
        sine[i] = (int) ((sin_value * (float) sin_multiplier) + 0.1f);
    }
}

int trueModulo(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

static SwampFixed32 sinHelper(SwampFixed32 fixedRadian)
{
    fixedRadian = trueModulo(fixedRadian, SWAMP_CORE_MATH_TWO_PI_FIXED);
    int indexInArray = fixedRadian * SWAMP_CORE_SIN_TABLE_MAX / SWAMP_CORE_MATH_TWO_PI_FIXED;
    indexInArray = indexInArray % SWAMP_CORE_SIN_TABLE_MAX;

    if (!g_sin_table_initialized) {
        init_sin_table(g_sin_table);
        g_sin_table_initialized = 1;
    }

    int sin_value = g_sin_table[indexInArray];
    int result = sin_value * SWAMP_FIXED_FACTOR / sin_multiplier;

    return result;
}

static SwampInt32 fixedAngleToDegree(SwampFixed32 angle)
{
    return (angle * MAX_DEGREE) / MAX_FIXED_RADIAN;
}

static SwampFixed32 degreeToFixedAngle(SwampInt32 degreeAngle)
{
    return (degreeAngle * MAX_FIXED_RADIAN) / MAX_DEGREE;
}

void swampCoreMathSin(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* angle)
{
    *result = sinHelper(*angle);
}

void swampCoreMathCos(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* angle)
{
    const int quarterTurn = (628 / 4);
    *result = sinHelper(*angle + quarterTurn);
}

void swampCoreMathRnd(SwampInt32* result, SwampMachineContext* context, const SwampInt32* basis, const SwampInt32* modulus)
{
    *result = (int) pseudoRandom(*basis, *modulus + 1);
}

void swampCoreMathATan(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* y, const SwampFixed32* x)
{
    SwampInt32 yInt = fixedAngleToDegree(y);
    SwampInt32 xInt = fixedAngleToDegree(x);

    int fast_degrees = fastAtan2(yInt, xInt);

    *result = degreeToFixedAngle(fast_degrees);
}

void swampCoreMathMid(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* a, const SwampFixed32* b)
{
    *result = (*b + *a) / 2;
}

void swampCoreMathAbs(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* a)
{
    if (*a >= 0) {
        *result = *a;
        return;
    }

    *result = -*a;
}

void swampCoreMathSign(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* a)
{
    if (*a == 0) {
        *result = 0;
        return;
    }

    if (*a > 0) {
        *result = 1;
    } else {
        *result = -1;
    }
}

static int clamp(int v, int min, int max)
{
    return (v < min ? min : (v > max ? max : v));
}

void swampCoreMathClamp(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* min, const SwampFixed32* max, const SwampFixed32* v)
{
    *result = clamp(*v, *min, *max);
}

void swampCoreMathLerp(SwampInt32* result, SwampMachineContext* context, const SwampFixed32* t, const SwampInt32* start, const SwampInt32* end)
{
    *result = (*start * SWAMP_FIXED_FACTOR + *t * (*end - *start)) / SWAMP_FIXED_FACTOR;
}

void swampCoreMathMetronome(SwampBool* result, SwampMachineContext* context, const SwampInt32* t, const SwampInt32* cycleCount, const SwampInt32* enableCount, const SwampInt32* offset)
{
    int cycleFrameNumber = (*offset + *t) % *cycleCount;
    *result = cycleFrameNumber < *enableCount;
}

void swampCoreMathRandomDelta(SwampBool* result, SwampMachineContext* context, const SwampInt32* t, const SwampInt32* value, const SwampInt32* randomMaxDelta)
{
    int span = (*randomMaxDelta * 2) + 1;
    int delta = ((int) pseudoRandom(*t, span)) - *randomMaxDelta;
    *result = value + delta;
}

static int mod(int v, int div)
{
    if (div < 0) {
        div = -div;
    }
    int ret = v % div;
    if (ret < 0) {
        ret += div;
    }
    return ret;
}


// Same as in C99. Remainder, not euclidean mod. Negative numbers return negative results.
void swampCoreMathRemainderBy(SwampInt32* result, SwampMachineContext* context, const SwampInt32* divider, const SwampInt32* value)
{
    if (*divider == 0) {
        SWAMP_LOG_ERROR("Error remainderBy can not handle 0");
    }

    int remainder = mod(*value, *divider);

    *result = remainder;
}

// Euclidean mod. Always returns positive results.
void swampCoreMathMod(SwampInt32* result, SwampMachineContext* context, const SwampInt32* divider, const SwampInt32* value)
{
    if (*divider == 0) {
        SWAMP_LOG_SOFT_ERROR("Error modBy() can not handle 0");
        *result = 0;
        return;
    }

    int remainder = mod(*value, *divider);

    *result = remainder;
}

void* swampCoreMathFindFunction(const char* fullyQualifiedName)
{
    SwampBindingInfo info[] = {
        {"Math.remainderBy", swampCoreMathRemainderBy},
    };

    for (size_t i = 0; i < sizeof(info) / sizeof(info); ++i) {
        if (tc_str_equal(info[i].name, fullyQualifiedName) == 0) {
            return info[i].fn;
        }
    }
    // SWAMP_LOG_INFO("didn't find: %s", function_name);
    return 0;
}
