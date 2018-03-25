#include "Halftone.hpp"

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
	Vector vec(xL, yL);

	// sample colour grid
	Vector channel_0 = vec.getProjected(info->origin, info->normal_0, info->grid_step, info->grid_half_step);
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(channel_0.x), D2FIX(channel_0.y), &info->samp_pb, outP));
	double blackness = 1.0 - (outP->red + outP->green + outP->blue) / 765.0;
	double radius_0 = blackness * info->grid_step;
	bool ch0 = vec.getDistanceTo(channel_0) <= radius_0;

	Vector channel_1 = vec.getProjected(info->origin, info->normal_1, info->grid_step, info->grid_half_step);
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(channel_1.x), D2FIX(channel_1.y), &info->samp_pb, outP));
	double cyan = (outP->blue + outP->green) / 2.0;
	double cyanness = max(0.0, min(1.0, (cyan - outP->red) / 128.0));
	double radius_1 = cyanness * info->grid_step * blackness;
	bool ch1 = vec.getDistanceTo(channel_1) <= radius_1;

	Vector channel_2 = vec.getProjected(info->origin, info->normal_2, info->grid_step, info->grid_half_step);
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(channel_2.x), D2FIX(channel_2.y), &info->samp_pb, outP));
	double magenta = (outP->red + outP->blue) / 2.0;
	double magentaness = max(0.0, min(1.0, (magenta - outP->green) / 128.0));
	double radius_2 = magentaness * info->grid_step * blackness;
	bool ch2 = vec.getDistanceTo(channel_2) <= radius_2;

	Vector channel_3 = vec.getProjected(info->origin, info->normal_3, info->grid_step, info->grid_half_step);
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(channel_3.x), D2FIX(channel_3.y), &info->samp_pb, outP));
	double yellow = (outP->red + outP->green) / 2.0;
	double yellowness = max(0.0, min(1.0, (yellow - outP->blue) / 128.0));
	double radius_3 = yellowness * info->grid_step * blackness;
	bool ch3 = vec.getDistanceTo(channel_3) <= radius_3;


	if (!ch0) {	
		outP->red = (ch1) ? 0 : 255;
		outP->green = (ch2) ? 0 : 255;
		outP->blue = (ch3) ? 0 : 255;
	} else {
		outP->red = 0;
		outP->green = 0;
		outP->blue = 0;
	}

	outP->alpha = inP->alpha;

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
	info.grid_step = (double)params[PARAM_RADIUS]->u.fs_d.value;
	info.grid_half_step = info.grid_step * 0.5;
	info.angle_0 = FIX2D(params[PARAM_ANGLE_0]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.angle_1 = FIX2D(params[PARAM_ANGLE_1]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.angle_2 = FIX2D(params[PARAM_ANGLE_2]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.angle_3 = FIX2D(params[PARAM_ANGLE_3]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.greyscale = PF_Boolean((params[PARAM_USE_GREYSCALE]->u.bd.value));
	info.in_data = *in_data;
	info.ref = in_data->effect_ref;
	info.samp_pb.src = inputP;
	info.samp_pb.x_radius = D2FIX(min(2.0, info.grid_step));
	info.samp_pb.y_radius = D2FIX(min(2.0, info.grid_step));

	// make vectors
	double centre_x = inputP->width * 0.5 - fmod(inputP->width * 0.5, info.grid_step);
	double centre_y = inputP->height * 0.5 - fmod(inputP->height * 0.5, info.grid_step);
	info.origin.set(centre_x, centre_y);
	info.normal_0.set(cos(info.angle_0 + HALF_PI), sin(info.angle_0 + HALF_PI));
	info.normal_1.set(cos(info.angle_1 + HALF_PI), sin(info.angle_1 + HALF_PI));
	info.normal_2.set(cos(info.angle_2 + HALF_PI), sin(info.angle_2 + HALF_PI));
	info.normal_3.set(cos(info.angle_3 + HALF_PI), sin(info.angle_3 + HALF_PI));

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
	PF_ADD_FLOAT_SLIDER("Size", 0, 256, 0, 32, 0, 6, 0, 0, 0, PARAM_RADIUS);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Black", 45, PARAM_ANGLE_0);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 1", 75, PARAM_ANGLE_1);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 2", 15, PARAM_ANGLE_2);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 3", 0, PARAM_ANGLE_3);
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
