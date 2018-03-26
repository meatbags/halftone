#include "Halftone.hpp"

static PF_Err ColourPixel8(
	PF_Pixel8 *outP,
	PF_Pixel8 *samp_c,
	PF_Pixel8 *samp_m,
	PF_Pixel8 *samp_y,
	Vector *point,
	Vector *dot_c,
	Vector *dot_m,
	Vector *dot_y,
	double step
) {
	PF_Err err = PF_Err_NONE;

	// inverse channel strength
	double red_inv = max(0.0, (((samp_c->green + samp_c->blue) / 2.0) - samp_c->red) / 255.0);
	double green_inv = max(0.0, (((samp_m->red + samp_m->blue) / 2.0) - samp_m->green) / 255.0);
	double blue_inv = max(0.0, (((samp_y->red + samp_y->green) / 2.0) - samp_y->blue) / 255.0);

	// scale by lightness & saturation, cyan sample
	double cmax = max(samp_c->red, max(samp_c->green, samp_c->blue)) / 255.0;
	double cmin = min(samp_c->red, min(samp_c->green, samp_c->blue)) / 255.0;
	double light = (cmax + cmin) * 0.5;
	double delta = cmax - cmin;
	double sat = (delta == 0.0) ? 0.0 : delta / (1 - abs(2 * light - 1));
	double offset_c = (1.0 - sat) * pow(1.0 - light, 2);
	
	// magenta sample
	cmax = max(samp_m->red, max(samp_m->green, samp_m->blue)) / 255.0;
	cmin = min(samp_m->red, min(samp_m->green, samp_m->blue)) / 255.0;
	light = (cmax + cmin) * 0.5;
	delta = cmax - cmin;
	sat = (delta == 0.0) ? 0.0 : delta / (1 - abs(2 * light - 1));
	double offset_m = (1.0 - sat) * pow(1.0 - light, 2);
	
	// yellow sample
	cmax = max(samp_y->red, max(samp_y->green, samp_y->blue)) / 255.0;
	cmin = min(samp_y->red, min(samp_y->green, samp_y->blue)) / 255.0;
	light = (cmax + cmin) * 0.5;
	delta = cmax - cmin;
	sat = (delta == 0.0) ? 0.0 : delta / (1 - abs(2 * light - 1));
	double offset_y = (1.0 - sat) * pow(1.0 - light, 2);

	// get radii
	double radius_c = step * (red_inv + offset_c);
	double radius_m = step * (green_inv + offset_m);
	double radius_y = step * (blue_inv + offset_y);
	double distance_c = point->getDistanceTo(dot_c);
	double distance_m = point->getDistanceTo(dot_m);
	double distance_y = point->getDistanceTo(dot_y);

	// apply colour
	if (distance_c < radius_c) {
		outP->red = (A_u_char)max(0.0, outP->red - CLAMP(0.0, 1.0, radius_c - distance_c) * 255);
	}
	if (distance_m < radius_m) {
		outP->green = (A_u_char)max(0.0, outP->green - CLAMP(0.0, 1.0, radius_m - distance_m) * 255);
	}
	if (distance_y < radius_y) {
		outP->blue = (A_u_char)max(0.0, outP->blue - CLAMP(0.0, 1.0, radius_y - distance_y) * 255);
	}

	return err;
}

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
	
	// refactor, simplify? rethink algorithm...

	// sample all colour grids
	SampleCell cell_c = getCell(vec.x, vec.y, info->origin, info->normal_1, info->grid_step, info->grid_half_step);
	SampleCell cell_m = getCell(vec.x, vec.y, info->origin, info->normal_2, info->grid_step, info->grid_half_step);
	SampleCell cell_y = getCell(vec.x, vec.y, info->origin, info->normal_3, info->grid_step, info->grid_half_step);
	cell_c.clamp(info->samp_pb.src->width - 1, info->samp_pb.src->height - 1);
	cell_m.clamp(info->samp_pb.src->width - 1, info->samp_pb.src->height - 1);
	cell_y.clamp(info->samp_pb.src->width - 1, info->samp_pb.src->height - 1);
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_c.s1.x), D2FIX(cell_c.s1.y), &info->samp_pb, &cell_c.sample1));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_m.s1.x), D2FIX(cell_m.s1.y), &info->samp_pb, &cell_m.sample1));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_y.s1.x), D2FIX(cell_y.s1.y), &info->samp_pb, &cell_y.sample1));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_c.s2.x), D2FIX(cell_c.s2.y), &info->samp_pb, &cell_c.sample2));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_m.s2.x), D2FIX(cell_m.s2.y), &info->samp_pb, &cell_m.sample2));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_y.s2.x), D2FIX(cell_y.s2.y), &info->samp_pb, &cell_y.sample2));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_c.s3.x), D2FIX(cell_c.s3.y), &info->samp_pb, &cell_c.sample3));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_m.s3.x), D2FIX(cell_m.s3.y), &info->samp_pb, &cell_m.sample3));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_y.s3.x), D2FIX(cell_y.s3.y), &info->samp_pb, &cell_y.sample3));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_c.s4.x), D2FIX(cell_c.s4.y), &info->samp_pb, &cell_c.sample4));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_m.s4.x), D2FIX(cell_m.s4.y), &info->samp_pb, &cell_m.sample4));
	ERR(suites.Sampling8Suite1()->subpixel_sample(in_data->effect_ref, D2FIX(cell_y.s4.x), D2FIX(cell_y.s4.y), &info->samp_pb, &cell_y.sample4));

	// reset output
	outP->alpha = inP->alpha;
	outP->red = PF_MAX_CHAN8;
	outP->green = PF_MAX_CHAN8;
	outP->blue = PF_MAX_CHAN8;

	// write to output (multiply)
	ERR(ColourPixel8(outP, &cell_c.sample1, &cell_m.sample1, &cell_y.sample1, &vec, &cell_c.p1, &cell_m.p1, &cell_y.p1, info->grid_step));
	ERR(ColourPixel8(outP, &cell_c.sample2, &cell_m.sample2, &cell_y.sample2, &vec, &cell_c.p2, &cell_m.p2, &cell_y.p2, info->grid_step));
	ERR(ColourPixel8(outP, &cell_c.sample3, &cell_m.sample3, &cell_y.sample3, &vec, &cell_c.p3, &cell_m.p3, &cell_y.p3, info->grid_step));
	ERR(ColourPixel8(outP, &cell_c.sample4, &cell_m.sample4, &cell_y.sample4, &vec, &cell_c.p4, &cell_m.p4, &cell_y.p4, info->grid_step));

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
	double centre_x = 0; //inputP->width * 0.5 - fmod(inputP->width * 0.5, info.grid_step);
	double centre_y = 0; //inputP->height * 0.5 - fmod(inputP->height * 0.5, info.grid_step);
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
	PF_ADD_FLOAT_SLIDER("Size", 0, 256, 0, 32, 0, 12, 0, 0, 0, PARAM_RADIUS);
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
