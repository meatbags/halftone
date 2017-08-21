#pragma once

#ifndef SKELETON_H
#define SKELETON_H

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

// version

#define	MAJOR_VERSION 1
#define	MINOR_VERSION 0
#define	BUG_VERSION	0
#define	STAGE_VERSION PF_Stage_DEVELOP
#define	BUILD_VERSION 1

// utilities

#define MAX_8 255
#define MAX_16 32767
#define D2FIX(x) (((long)x)<<16) 
#define FIX2D(x) (x / ((double)(1L << 16)))

// param ids

enum {
	INPUT_LAYER = 0,
	PARAM_RADIUS_MIN,
	PARAM_RADIUS_MAX,
	PARAM_ANGLE_K,
	PARAM_ANGLE_R,
	PARAM_ANGLE_G,
	PARAM_ANGLE_B,
	PARAM_USE_GREYSCALE,
	PARAM_COUNT
};

typedef struct Circle {
	double x;
	double y;
	double radius;
} Circle;

typedef struct ColourCircles {
	Circle K;
	Circle R;
	Circle G;
	Circle B;
} ColourCircles;

typedef struct Collisions {
	bool K;
	bool R;
	bool G;
	bool B;
	bool none;
} Collisions;

typedef struct Interface {
	double radius_min;
	double radius_max;
	double angle_k;
	double angle_r;
	double angle_g;
	double angle_b;
	int use_greyscale;
	PF_LayerDef *world;
	PF_InData *in_data;
	PF_SampPB samp_pb;
} Interface;

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

#endif // SKELETON_H