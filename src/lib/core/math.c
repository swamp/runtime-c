/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <math.h>
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/core/math.h>
#include <swamp-runtime/log.h>
#include <swamp-runtime/swamp.h>

#define FIXED_MULTIPLIER (1000)
const int MAX_FIXED_RADIAN = 6280;
const int MAX_DEGREE = 360;

static int fast_atan2(signed int x, signed int y)
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

static unsigned int pseudo_random(unsigned int start, unsigned int modulus)
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
static const swamp_fixed32 SWAMP_CORE_MATH_TWO_PI_FIXED = MAX_FIXED_RADIAN;

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

static int sin_helper(swamp_fixed32 fixedRadian)
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

static swamp_int32 fixedAngleToDegree(swamp_fixed32 angle)
{
    return (angle * MAX_DEGREE) / MAX_FIXED_RADIAN;
}

static swamp_fixed32 degreeToFixedAngle(swamp_int32 degreeAngle)
{
    return (degreeAngle * MAX_FIXED_RADIAN) / MAX_DEGREE;
}

static const swamp_value* sin_helper_alloc(swamp_allocator* allocator, swamp_fixed32 fixedRadian)
{
    int r = sin_helper(fixedRadian);
    const swamp_value* r_value = swamp_allocator_alloc_fixed(allocator, r);

    return r_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_sin)
{
    const swamp_fixed32 angle = swamp_value_fixed(arguments[0]);

    return sin_helper_alloc(allocator, angle);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_cos)
{
    const swamp_fixed32 angle = swamp_value_int(arguments[0]);
    const int quarterTurn = (628 / 4);
    return sin_helper_alloc(allocator, angle + quarterTurn);
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_rnd)
{
    const swamp_int32 basis = swamp_value_int(arguments[0]);
    const swamp_int32 modulus = swamp_value_int(arguments[1]) + 1;

    int result_value = (int) pseudo_random(basis, modulus);
    const swamp_value* r_value = swamp_allocator_alloc_fixed(allocator, result_value);
    return r_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_atan2)
{
    swamp_int32 y_int = fixedAngleToDegree(swamp_value_fixed(arguments[0]));
    swamp_int32 x_int = fixedAngleToDegree(swamp_value_fixed(arguments[1]));

    int fast_degrees = fast_atan2(y_int, x_int);

    swamp_fixed32 fixedAngle = degreeToFixedAngle(fast_degrees);
    const swamp_value* r_value = swamp_allocator_alloc_fixed(allocator, fixedAngle);
    return r_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_mid)
{
    swamp_int32 a = swamp_value_int(arguments[0]);
    swamp_int32 b = swamp_value_int(arguments[1]);

    int mid = (b + a) / 2;
    const swamp_value* r_value = swamp_allocator_alloc_integer(allocator, mid);
    return r_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_abs)
{
    swamp_int32 a = swamp_value_int(arguments[0]);
    if (a >= 0) {
        return arguments[0];
    }

    const swamp_value* r_value = swamp_allocator_alloc_integer(allocator, -a);
    return r_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_sign)
{
    swamp_int32 a = swamp_value_int(arguments[0]);
    if (a == 0) {
        return arguments[0];
    }

    if (a > 0) {
        a = 1;
    } else {
        a = -1;
    }

    const swamp_value* r_value = swamp_allocator_alloc_integer(allocator, a);
    return r_value;
}

static int clamp(int v, int min, int max)
{
    return (v < min ? min : (v > max ? max : v));
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_clamp)
{
    swamp_int32 min = swamp_value_int(arguments[0]);
    swamp_int32 max = swamp_value_int(arguments[1]);
    swamp_int32 v = swamp_value_int(arguments[2]);
    int clamped = clamp(v, min, max);
    // SWAMP_LOG_INFO("clamped %d %d %d", min, max, v);
    const swamp_value* r_value = swamp_allocator_alloc_integer(allocator, clamped);
    return r_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_lerp)
{
    int t = swamp_value_fixed(arguments[0]);
    int start = swamp_value_int(arguments[1]);
    int end = swamp_value_int(arguments[2]);

    int lerped = (start * SWAMP_FIXED_FACTOR + t * (end - start)) / SWAMP_FIXED_FACTOR;

    const swamp_value* r_value = swamp_allocator_alloc_integer(allocator, lerped);
    return r_value;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_metronome)
{
    int t = swamp_value_int(arguments[0]);
    int cycle_count = swamp_value_int(arguments[1]);
    int enabled_count = swamp_value_int(arguments[2]);
    int offset = swamp_value_int(arguments[3]);

    int cycle_frame_number = (offset + t) % cycle_count;
    swamp_bool enabled = cycle_frame_number < enabled_count;
    const swamp_value* result = swamp_allocator_alloc_boolean(allocator, enabled);
    return result;
}

SWAMP_FUNCTION_EXPOSE(swamp_core_math_drunk)
{
    int t = swamp_value_int(arguments[0]);
    int value = swamp_value_int(arguments[1]);
    int random_max_delta = swamp_value_int(arguments[2]);

    int span = (random_max_delta * 2) + 1;
    int delta = ((int) pseudo_random(t, span)) - random_max_delta;
    int result_value = value + delta;

    const swamp_value* r_value = swamp_allocator_alloc_integer(allocator, result_value);
    return r_value;
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
SWAMP_FUNCTION_EXPOSE(swamp_core_math_remainder_by)
{
    int divider = swamp_value_int(arguments[0]);
    int value = swamp_value_int(arguments[1]);

    if (divider == 0) {
        SWAMP_LOG_SOFT_ERROR("Error remainderBy can not handle 0");
        return 0;
    }

    int remainder = mod(value, divider);
    const swamp_value* r_value = swamp_allocator_alloc_integer(allocator, remainder);
    return r_value;
}

// Euclidean mod. Always returns positive results.
SWAMP_FUNCTION_EXPOSE(swamp_core_math_mod)
{
    int divider = swamp_value_int(arguments[0]);
    int value = swamp_value_int(arguments[1]);

    if (divider == 0) {
        SWAMP_LOG_SOFT_ERROR("Error modBy() can not handle 0");
        return 0;
    }

    int remainder = mod(value, divider);
    const swamp_value* r_value = swamp_allocator_alloc_integer(allocator, remainder);
    return r_value;
}
