#pragma once

#ifndef HALFTONE_H
#define HALFTONE_H

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned short u_int16;
typedef unsigned long u_long;
typedef short int int16;

#define PF_TABLE_BITS 12
#define PF_TABLE_SZ_16 4096
#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"
#include <math.h>

// version

#define	MAJOR_VERSION 1
#define	MINOR_VERSION 0
#define	BUG_VERSION	0
#define	STAGE_VERSION PF_Stage_DEVELOP
#define	BUILD_VERSION 1

// utilities

#define MAX_8 255
#define MAX_16 32767
#define PI 3.141593
#define D2FIX(x) (((long)x)<<16)
#define FIX2D(x) (x / ((double)(1L << 16)))
#define SAMPLE_KEY 0x01
#define SAMPLE_RED 0x02
#define SAMPLE_GREEN 0x03
#define SAMPLE_BLUE 0x04
#define HUE_RED 0
#define HUE_GREEN 2.0944
#define HUE_BLUE 4.18879

// params

enum {
	INPUT_LAYER = 0,
	PARAM_RADIUS_MAX,
	PARAM_ANGLE_K,
	PARAM_ANGLE_R,
	PARAM_ANGLE_G,
	PARAM_ANGLE_B,
	PARAM_USE_GREYSCALE,
	PARAM_COUNT
};

// types

typedef struct Point {
	double x;
	double y;
} Point;

typedef struct Point16 {
	Point p;
	double radius;
	PF_Pixel16 pixel;
	double distance;
} Point16;

typedef struct SamplePoints16 {
	int colour_key;
	Point16 p0;
	Point16 p1;
	Point16 p2;
	Point16 p3;
	bool collision;
} SamplePoints16;

typedef struct Collisions {
	bool K;
	bool R;
	bool G;
	bool B;
	bool none;
} Collisions;

typedef struct Interface {
	double radius_max;
	double angle_k;
	double angle_r;
	double angle_g;
	double angle_b;
	int use_greyscale;
	PF_LayerDef *world;
	PF_EffectWorld effect_world;
	PF_InData *in_data;
	PF_SampPB samp_pb;
} Interface;

// methods

PF_Pixel16 
BlankPixel16 (
	A_u_short R,
	A_u_short B,
	A_u_short G
) {
	PF_Pixel16 p;

	p.alpha = MAX_16;
	p.red = R;
	p.blue = B;
	p.green = G;

	return p;
}

static PF_Err
BlankPoint16 (
	Point16 &p
) {
	PF_Err err = PF_Err_NONE;

	p.p.x = 0;
	p.p.y = 0;
	p.radius = 0;
	p.distance = 0;
	p.pixel.alpha = 0;
	p.pixel.red = 0;
	p.pixel.blue = 0;
	p.pixel.green = 0;

	return err;
}

Point RotateCoordinates (
	double x,
	double y,
	double centre_x,
	double centre_y,
	double rotate
) {
	double mag, angle;
	Point p;

	mag = sqrt(pow(x - centre_x, 2) + pow(y - centre_y, 2));
	angle = atan2(y - centre_y, x - centre_x) + rotate;
	p.x = centre_x + mag * cos(angle);
	p.y = centre_y + mag * sin(angle);

	return p;
}

double easing (
	double x
) {
	return x * x;
}

static PF_Err
Colour16Radius (
	Point16 &p,
	PF_Pixel16 &pixel,
	int key,
	double size
) {
	PF_Err err = PF_Err_NONE;

	// convert to radius
	if (key == SAMPLE_KEY) {
		p.radius = size * (easing(1.0 - ((pixel.red + pixel.blue + pixel.green) / 3.) / (double)(MAX_16)));
	} else {
		// get HLS
		double R, G, B, H, L, S, MAX, MIN, ANGLE, DIFF;

		R = pixel.red / (double)MAX_16;
		G = pixel.green / (double)MAX_16;
		B = pixel.blue / (double)MAX_16;
		MAX = max(max(R, B), G);
		MIN = min(min(R, B), G);
		H = L = S = (MAX + MIN) / 2;
	
		if (MAX == MIN) {
			H = S = 0;
		} else {
			double D = MAX - MIN;
			S = (L > 0.5) ? D / (2 - MAX - MIN) : D / (MAX + MIN);
		
			if (MAX == R) {
				H = (G - B) / D + ((G < B) ? 6 : 0);
			} else if (MAX == G) {
				H = (B - R) / D + 2;
			} else if (MAX == B) {
				H = (R - G) / D + 4;
			}

			H = (H / 6) * PI * 2;
		}

		// get hue difference
		ANGLE = (key == SAMPLE_RED) ? HUE_RED : ((key == SAMPLE_BLUE) ? HUE_BLUE : HUE_GREEN );
		DIFF = 1 - abs(atan2(sin(H - ANGLE), cos(H - ANGLE))) / PI;

		// limit

		p.radius = size * S * DIFF;
	}

	return err;
}

static PF_Err
DistanceToRadius16 (
	Point16 &point,
	double x,
	double y
) {
	PF_Err err = PF_Err_NONE;
	
	double mag = sqrt(pow(x - point.p.x, 2) + pow(y - point.p.y, 2));
	point.distance = mag - point.radius;

	return err;
}

SamplePoints16 
SampleGrid16 (
	int key,
	double grid_angle,
	double x,
	double y,
	Interface* master,
	AEGP_SuiteHandler* suites
) {
	SamplePoints16 sample;
	Point rotated, start, stop;
	double centre_x, centre_y, size;

	// initialise
	BlankPoint16(sample.p0);
	BlankPoint16(sample.p1);
	BlankPoint16(sample.p2);
	BlankPoint16(sample.p3);

	// rotate coordinates
	centre_x = (double)master->world->width / 2.;
	centre_y = (double)master->world->height / 2.;
	rotated = RotateCoordinates(x, y, centre_x, centre_y, -grid_angle);
	start.x = rotated.x - fmod(rotated.x + master->world->width, master->radius_max);
	start.y = rotated.y - fmod(rotated.y + master->world->height, master->radius_max);
	stop.x = start.x + master->radius_max;
	stop.y = start.y + master->radius_max;
	
	// get sample points
	sample.p0.p = RotateCoordinates(start.x, start.y, centre_x, centre_y, grid_angle);
	sample.p1.p = RotateCoordinates(stop.x, start.y, centre_x, centre_y, grid_angle);
	sample.p2.p = RotateCoordinates(stop.x, stop.y, centre_x, centre_y, grid_angle);
	sample.p3.p = RotateCoordinates(start.x, stop.y, centre_x, centre_y, grid_angle);
	
	// get colours
	
	suites->Sampling16Suite1()->area_sample16(master->in_data->effect_ref, D2FIX(sample.p0.p.x), D2FIX(sample.p0.p.y), &master->samp_pb, &sample.p0.pixel);
	suites->Sampling16Suite1()->area_sample16(master->in_data->effect_ref, D2FIX(sample.p1.p.x), D2FIX(sample.p1.p.y), &master->samp_pb, &sample.p1.pixel);
	suites->Sampling16Suite1()->area_sample16(master->in_data->effect_ref, D2FIX(sample.p2.p.x), D2FIX(sample.p2.p.y), &master->samp_pb, &sample.p2.pixel);
	suites->Sampling16Suite1()->area_sample16(master->in_data->effect_ref, D2FIX(sample.p3.p.x), D2FIX(sample.p3.p.y), &master->samp_pb, &sample.p3.pixel);
	
	// process radii
	
	size = master->radius_max * 0.75;
	Colour16Radius(sample.p0, sample.p0.pixel, key, size);
	Colour16Radius(sample.p1, sample.p1.pixel, key, size);
	Colour16Radius(sample.p2, sample.p2.pixel, key, size);
	Colour16Radius(sample.p3, sample.p3.pixel, key, size);
	
	// get collisions
	DistanceToRadius16(sample.p0, x, y);
	DistanceToRadius16(sample.p1, x, y);
	DistanceToRadius16(sample.p2, x, y);
	DistanceToRadius16(sample.p3, x, y);

	// set key
	sample.colour_key = key;
	sample.collision = (
		sample.p0.distance < 0 ||
		sample.p1.distance < 0 ||
		sample.p2.distance < 0 ||
		sample.p3.distance < 0
	);

	return sample;
}

bool CheckCollisions (
	SamplePoints16 &R,
	SamplePoints16 &B,
	SamplePoints16 &G,
	SamplePoints16 &K
) {
	return (R.collision || B.collision || G.collision || K.collision);
}

double GetAA16 (
	SamplePoints16 sample
) {
	double aa = 1;

	if (sample.p0.distance >= 0 && sample.p1.distance >= 0 && sample.p2.distance >= 0 && sample.p3.distance >= 0) {
		aa = 0;
	} else if (sample.p0.distance < -1 || sample.p1.distance < -1 || sample.p2.distance < -1 || sample.p3.distance < -1) {
		aa = 1;
	} else {
		double a = (sample.p0.distance >= 0) ? 0 : easing(-sample.p0.distance);
		double b = (sample.p1.distance >= 0) ? 0 : easing(-sample.p1.distance);
		double c = (sample.p2.distance >= 0) ? 0 : easing(-sample.p2.distance);
		double d = (sample.p3.distance >= 0) ? 0 : easing(-sample.p3.distance);
		aa = min(1, a + b + c + d);
	}

	return aa;
}

static PF_Err
WriteToOutput16 (
	SamplePoints16 sample,
	PF_Pixel16 &out
) {
	PF_Err err = PF_Err_NONE;
	double aa = GetAA16(sample);

	switch (sample.colour_key) {
		case (SAMPLE_KEY):
			out.red -= (A_u_short)floor(aa * out.red);
			out.blue -= (A_u_short)floor(aa * out.blue);
			out.green -= (A_u_short)floor(aa * out.green);
			break;
		case (SAMPLE_RED):
			out.red = (A_u_short)floor(aa * MAX_16);
			break;
		case (SAMPLE_BLUE):
			out.blue = (A_u_short)floor(aa * MAX_16);
			break;
		case (SAMPLE_GREEN):
			out.green = (A_u_short)floor(aa * MAX_16);
			break;
		default:
			break;
	}

	return err;
}

#ifdef __cplusplus
	extern "C" {
#endif

DllExport	PF_Err 
EntryPointFunc(	
	PF_Cmd cmd,
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef	*output,
	void *extra) ;

#ifdef __cplusplus
}
#endif

#endif // HALFTONE_H