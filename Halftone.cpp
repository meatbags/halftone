#include "Halftone.hpp"
#include <math.h>

static PF_Err Halftone8(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8 *inP,
	PF_Pixel8 *outP
) {
	PF_Err err = PF_Err_NONE;
	register HalftoneInfo *info = (HalftoneInfo*)refcon;
	PF_InData *in_data = &(info->in_data);
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_Fixed xF = D2FIX(xL) - (D2FIX(xL) % info->radiusF);
	PF_Fixed yF = D2FIX(yL) - (D2FIX(yL) % info->radiusF);
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, xF, yF, &info->samp_pb, outP));
	double samp_radiusF = (outP->red + outP->green + outP->blue) / double(3 * PF_MAX_CHAN8) * FIX2D(info->radiusF);
	
	if (hypot(double(xF - D2FIX(xL)), double(yF - D2FIX(yL))) > D2FIX(samp_radiusF)) {
		outP->alpha = PF_MAX_CHAN8;
		outP->red = PF_MAX_CHAN8;
		outP->green = PF_MAX_CHAN8;
		outP->blue = PF_MAX_CHAN8;
	} else {
		outP->alpha = 0;
		outP->red = 0;
		outP->green = 0;
		outP->blue = 0;
	}

	return err;
}

static PF_Err
Halftone16(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel16 *inP,
	PF_Pixel16 *outP
) {
	PF_Err err = PF_Err_NONE;
	register HalftoneInfo *info = (HalftoneInfo*)refcon;
	PF_InData *in_data = &(info->in_data);
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_Fixed xF = xL << 16;
	PF_Fixed yF = yL << 16;

	ERR(
		suites.Sampling16Suite1()->subpixel_sample16(
			in_data->effect_ref,
			xF,
			yF,
			&info->samp_pb,
			outP
		)
	);

	return err;
}

static PF_Err
Render(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output
) {
	PF_Err err = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	A_long linesL = output->extent_hint.bottom - output->extent_hint.top;
	HalftoneInfo info;
	PF_EffectWorld *inputP = &params[INPUT_LAYER]->u.ld;

	// get user options
	AEFX_CLR_STRUCT(info);
	info.radiusF = D2FIX(params[PARAM_RADIUS]->u.fs_d.value);
	info.angle_kF = params[PARAM_ANGLE_K]->u.ad.value * D2FIX(PF_RAD_PER_DEGREE);
	info.angle_rF = params[PARAM_ANGLE_R]->u.ad.value * D2FIX(PF_RAD_PER_DEGREE);
	info.angle_gF = params[PARAM_ANGLE_G]->u.ad.value * D2FIX(PF_RAD_PER_DEGREE);
	info.angle_bF = params[PARAM_ANGLE_B]->u.ad.value * D2FIX(PF_RAD_PER_DEGREE);
	info.greyscaleB = PF_Boolean((params[PARAM_USE_GREYSCALE]->u.bd.value));
	info.in_data = *in_data;
	info.ref = in_data->effect_ref;
	info.samp_pb.src = inputP;
	info.samp_pb.x_radius = min(8, info.radiusF);
	info.samp_pb.y_radius = min(8, info.radiusF);

	if (PF_WORLD_IS_DEEP(output)) {
		ERR(suites.Iterate16Suite1()->iterate(in_data, 0, linesL, inputP, NULL, (void*)&info, Halftone16, output));
	} else {
		ERR(suites.Iterate8Suite1()->iterate(in_data, 0, linesL, inputP, NULL, (void*)&info, Halftone8, output));
	}

	return err;
}

static PF_Err About(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", "Colour Halftone", MAJOR_VERSION, MINOR_VERSION, "Colour halftone functions.");

	return PF_Err_NONE;
}

static PF_Err GlobalSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
) {
	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;
	out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION, BUG_VERSION, STAGE_VERSION, BUILD_VERSION);

	return PF_Err_NONE;
}

static PF_Err ParamsSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
) {
	PF_ParamDef	def;
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Radius", 0, 128, 0, 32, 0, 16, 0, 0, 0, PARAM_RADIUS);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Black", 45, PARAM_ANGLE_K);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Red", 75, PARAM_ANGLE_R);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Green", 15, PARAM_ANGLE_G);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Blue", 0, PARAM_ANGLE_B);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Greyscale", 0, 0, PARAM_USE_GREYSCALE);
	out_data->num_params = PARAM_COUNT;

	return PF_Err_NONE;
}

DllExport
PF_Err
EntryPointFunc(
	PF_Cmd cmd,
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output,
	void *extra
) {
	PF_Err err = PF_Err_NONE;
	try {
		switch (cmd) {
		case PF_Cmd_ABOUT:
			err = About(in_data, out_data, params, output);
			break;
		case PF_Cmd_GLOBAL_SETUP:
			err = GlobalSetup(in_data, out_data, params, output);
			break;
		case PF_Cmd_PARAMS_SETUP:
			err = ParamsSetup(in_data, out_data, params, output);
			break;
		case PF_Cmd_RENDER:
			err = Render(in_data, out_data, params, output);
			break;
		}
	}
	catch (PF_Err &thrown_err) {
		err = thrown_err;
	}

	return err;
}
