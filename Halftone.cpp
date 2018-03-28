#include "Halftone.hpp"
#include "HalftoneSampler.hpp"

static PF_Err Halftone8(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8 *inP,
	PF_Pixel8 *outP
) {
	PF_Err err = PF_Err_NONE;
	register HalftoneInfo *info = (HalftoneInfo*)refcon;
	PF_InData *in_data = &info->in_data;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_Sampling8Suite1 *sampling_suite = suites.Sampling8Suite1();
	Vector vec(xL, yL);

	// sample all colour grids
	Sampler sampler_c = getSampler(vec.x, vec.y, info->origin, info->normal_1, info->grid_step, info->grid_half_step);
	Sampler sampler_m = getSampler(vec.x, vec.y, info->origin, info->normal_2, info->grid_step, info->grid_half_step);
	Sampler sampler_y = getSampler(vec.x, vec.y, info->origin, info->normal_3, info->grid_step, info->grid_half_step);
	ERR(sampler_c.sample(sampling_suite, in_data->effect_ref, info));
	ERR(sampler_m.sample(sampling_suite, in_data->effect_ref, info));
	ERR(sampler_y.sample(sampling_suite, in_data->effect_ref, info));

	// reset output, write channels
	outP->alpha = inP->alpha;
	outP->red = PF_MAX_CHAN8;
	outP->green = PF_MAX_CHAN8;
	outP->blue = PF_MAX_CHAN8;
	ERR(sampler_c.write8(1, &outP->red, vec, info->mode, info->grid_step, info->aa));
	ERR(sampler_m.write8(2, &outP->green, vec, info->mode, info->grid_step, info->aa));
	ERR(sampler_y.write8(3, &outP->blue, vec, info->mode, info->grid_step, info->aa));

	if (info->greyscale) {
		A_u_char average = (A_u_char)floor((outP->red + outP->green + outP->blue) / 3.0);
		outP->red = average;
		outP->green = average;
		outP->blue = average;
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
	info.mode = (A_u_char)params[PARAM_MODE]->u.pd.value;
	info.grid_step = ((double)params[PARAM_RADIUS]->u.fs_d.value) * ((double)inputP->width / (double)in_data->width);
	info.grid_half_step = info.grid_step * 0.5;
	info.aa = max(1.0, (double)params[PARAM_AA]->u.fs_d.value);
	info.angle_0 = FIX2D(params[PARAM_ANGLE_0]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.angle_1 = FIX2D(params[PARAM_ANGLE_1]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.angle_2 = FIX2D(params[PARAM_ANGLE_2]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.angle_3 = FIX2D(params[PARAM_ANGLE_3]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.greyscale = PF_Boolean((params[PARAM_USE_GREYSCALE]->u.bd.value));
	info.in_data = *in_data;
	info.ref = in_data->effect_ref;
	info.samp_pb.src = inputP;
	info.samp_pb.x_radius = D2FIX(2.0);
	info.samp_pb.y_radius = D2FIX(2.0);

	// get grid vectors
	double centre_x = 0; // inputP->width * 0.5 - fmod(inputP->width * 0.5, info.grid_step);
	double centre_y = 0; // inputP->height * 0.5 - fmod(inputP->height * 0.5, info.grid_step);
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
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", "Colour Halftone", MAJOR_VERSION, MINOR_VERSION, "Colour halftone operations by @meatbags");

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
	PF_ADD_POPUP("Mode", 7, 1, "Round|Square|Euclidean Dot|Ellipse|Line|Diamond|Flower", PARAM_MODE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Size", 0, 256, 0, 32, 0, 12, 0, 0, 0, PARAM_RADIUS);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Soften", 1, 256, 1, 32, 0, 1, 0, 0, 0, PARAM_AA);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Sample", 30, PARAM_ANGLE_0);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 1", 162, PARAM_ANGLE_1);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 2", 109, PARAM_ANGLE_2);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 3", 45, PARAM_ANGLE_3);
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
