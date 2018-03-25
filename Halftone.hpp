#pragma once
#ifndef HALFTONE_H
#define HALFTONE_H

#define PF_TABLE_BITS 12
#define PF_TABLE_SZ_16 4096
#define PF_DEEP_COLOR_AWARE 1
#include "AEConfig.h"

#ifdef AE_OS_WIN
typedef unsigned short PixelType;
#include <Windows.h>
#endif

#include <math.h>
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
#include "HalftoneMaths.hpp"

#define	MAJOR_VERSION 1
#define	MINOR_VERSION 0
#define	BUG_VERSION	0
#define	STAGE_VERSION PF_Stage_DEVELOP
#define	BUILD_VERSION 1

enum {
	INPUT_LAYER = 0,
	PARAM_RADIUS,
	PARAM_ANGLE_K,
	PARAM_ANGLE_R,
	PARAM_ANGLE_G,
	PARAM_ANGLE_B,
	PARAM_USE_GREYSCALE,
	PARAM_COUNT
};

typedef struct {
	double grid_stepD;
	double grid_half_stepD;
	double angle_kD;
	double angle_rD;
	double angle_gD;
	double angle_bD;
	PF_InData in_data;
	PF_SampPB samp_pb;
	PF_Boolean greyscaleB;
	PF_ProgPtr ref;
	Vector originV = Vector(0, 0);
	Vector normal_kV = Vector(0, 0);
	Vector normal_rV = Vector(0, 0);
	Vector normal_gV = Vector(0, 0);
	Vector normal_bV = Vector(0, 0);
} HalftoneInfo;

#ifdef __cplusplus
extern "C" {
#endif

	DllExport PF_Err
	EntryPointFunc(
		PF_Cmd cmd,
		PF_InData *in_data,
		PF_OutData *out_data,
		PF_ParamDef *params[],
		PF_LayerDef	*output,
		void *extra
	);

#ifdef __cplusplus
}
#endif
#endif // HALFTONE_H