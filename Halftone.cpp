#include "Halftone.h"
#include <math.h>

static PF_Err
About (
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
){
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	suites.ANSICallbacksSuite1()->sprintf(
		out_data->return_msg,
		"%s v%d.%d\r%s",
		"Colour Halftone", 
		MAJOR_VERSION,
		MINOR_VERSION,
		"Simple colour and greyscale halftone filter."
	);

	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
){
	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;
	out_data->my_version = PF_VERSION(
		MAJOR_VERSION,
		MINOR_VERSION,
		BUG_VERSION,
		STAGE_VERSION,
		BUILD_VERSION
	);

	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
){
	PF_ParamDef	def;
	
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Minimum Size", 0, 128, 0, 32, 0, 0, 0, 0, 0, PARAM_RADIUS_MIN);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Maximum Size", 0, 128, 0, 32, 0, 16, 0, 0, 0, PARAM_RADIUS_MAX);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 1", 45, PARAM_ANGLE_K);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 2", 75, PARAM_ANGLE_R);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 3", 15, PARAM_ANGLE_G);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Channel 4", 0, PARAM_ANGLE_B);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Black & White", 0, 0, PARAM_USE_GREYSCALE);

	out_data->num_params = PARAM_COUNT;
	
	return PF_Err_NONE;
}

static PF_Err
Halftone8 (
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8 *in,
	PF_Pixel8 *out
){
	PF_Err err = PF_Err_NONE;
	Interface *master = reinterpret_cast<Interface*>(refcon);
	AEGP_SuiteHandler suites(master->in_data->pica_basicP);

	return err;
}

bool insideCircle (
	Circle c,
	A_long x,
	A_long y
){
	double mag;

	mag = sqrt(pow(x - c.x, 2) + pow(y - c.y, 2));

	return (mag <= c.radius);
}

Collisions getCollisions (
	ColourCircles c,
	A_long x,
	A_long y
){
	Collisions result;

	result.K = insideCircle(c.K, x, y);
	result.R = insideCircle(c.R, x, y);
	result.G = insideCircle(c.G, x, y);
	result.B = insideCircle(c.B, x, y);
	result.none = (!result.K && !result.R && !result.B && !result.G);

	return result;
}

A_u_short compoundColour16 (
	bool b0,
	bool b1,
	bool b2,
	bool b3,
	A_u_short c0,
	A_u_short c1,
	A_u_short c2,
	A_u_short c3
) {
	c0 = c1 = c2 = c3 = MAX_16;
	long colour = (b0 ? (long)c0 : 0) + (b1 ? (long)c1 : 0) + (b2 ? (long)c2 : 0) + (b3 ? (long)c3 : 0);
	A_u_short result = (A_u_short)(min(MAX_16, colour));

	return result;
}

ColourCircles getCircles16 (
	PF_Pixel16 pixel,
	double x,
	double y,
	Interface *master
){
	ColourCircles result;
	double mag, cx, cy;
	double k, r, g, b;
	
	// get centre of area
	cx = x + master->radius_max / 2;
	cy = y + master->radius_max / 2;
	mag = master->radius_max / 4;

	// normalise KRGB values
	k = 1 - ((pixel.red + pixel.blue + pixel.green) / 3.) / (double)(MAX_16);
	r = pixel.red / (double)(MAX_16);
	b = pixel.blue / (double)(MAX_16);
	g = pixel.green / (double)(MAX_16);

	// get colour dots
	result.K.x = cx + cos(master->angle_k) * mag;
	result.K.y = cy - sin(master->angle_k) * mag;
	result.K.radius = k * master->radius_max;
	
	result.R.x = cx + cos(master->angle_r) * mag;
	result.R.y = cy - sin(master->angle_r) * mag;
	result.R.radius = r * master->radius_max;
	
	result.G.x = cx + cos(master->angle_g) * mag;
	result.G.y = cy - sin(master->angle_g) * mag;
	result.G.radius = g * master->radius_max;
	
	result.B.x = cx + cos(master->angle_b) * mag;
	result.B.y = cy - sin(master->angle_b) * mag;
	result.B.radius = b * master->radius_max;

	return result;
}

static PF_Err
Halftone16 (
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel16 *in,
	PF_Pixel16 *out
){
	PF_Err err = PF_Err_NONE;
	Interface *master = reinterpret_cast<Interface*>(refcon);
	AEGP_SuiteHandler suites(master->in_data->pica_basicP);

	if (master) {
		// get area bounds
		double x0 = (double)(xL - fmod(xL, master->radius_max));
		double y0 = (double)(yL - fmod(yL, master->radius_max));
		double x1 = (xL >= x0 + master->radius_max / 2) ? x0 + master->radius_max : x0 - master->radius_max;
		double y1 = (yL >= y0 + master->radius_max / 2) ? y0 + master->radius_max : y0 - master->radius_max;

		// sample area and relevant neighbours
		PF_Pixel16 p0, p1, p2, p3;

		suites.Sampling16Suite1()->area_sample16(master->in_data->effect_ref, D2FIX(x0), D2FIX(y0), &master->samp_pb, &p0);
		suites.Sampling16Suite1()->area_sample16(master->in_data->effect_ref, D2FIX(x1), D2FIX(y0), &master->samp_pb, &p1);
		suites.Sampling16Suite1()->area_sample16(master->in_data->effect_ref, D2FIX(x1), D2FIX(y1), &master->samp_pb, &p2);
		suites.Sampling16Suite1()->area_sample16(master->in_data->effect_ref, D2FIX(x0), D2FIX(y1), &master->samp_pb, &p3);

		// get KRGB dots
		ColourCircles c0 = getCircles16(p0, x0, y0, master);
		ColourCircles c1 = getCircles16(p1, x1, y0, master);
		ColourCircles c2 = getCircles16(p2, x1, y1, master);
		ColourCircles c3 = getCircles16(p3, x0, y1, master);

		// get collisions
		Collisions h0 = getCollisions(c0, xL, yL);
		Collisions h1 = getCollisions(c1, xL, yL);
		Collisions h2 = getCollisions(c2, xL, yL);
		Collisions h3 = getCollisions(c3, xL, yL);

		// write to pixel
		PF_Pixel16 output;

		if (h0.none && h1.none && h2.none && h3.none) {
			output.red = MAX_16;
			output.blue = MAX_16;
			output.green = MAX_16;
		} else if (h0.K || h1.K || h2.K || h3.K) {
			output.red = 0;
			output.blue = 0;
			output.green = 0;
		} else {
			output.red = compoundColour16(h0.R, h1.R, h2.R, h3.R, p0.red, p1.red, p2.red, p3.red);
			output.green = compoundColour16(h0.G, h1.G, h2.G, h3.G, p0.green, p1.green, p2.green, p3.green);
			output.blue = compoundColour16(h0.B, h1.B, h2.B, h3.B, p0.blue, p1.blue, p2.blue, p3.blue);
		}

		// place
		out->alpha = MAX_16;
		out->red = output.red;
		out->blue = output.blue;
		out->green = output.green;
	}

	return err;
}

static PF_Err
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output
){
	PF_Err err = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	A_long linesL = output->extent_hint.bottom - output->extent_hint.top;

	// interface
	Interface master;

	// get user input
	master.radius_min = (double)(params[PARAM_RADIUS_MIN]->u.fs_d.value);
	master.radius_max = (double)(params[PARAM_RADIUS_MAX]->u.fs_d.value);
	master.angle_k = (double)(FIX2D(params[PARAM_ANGLE_K]->u.ad.value)) * PF_RAD_PER_DEGREE;
	master.angle_r = (double)(FIX2D(params[PARAM_ANGLE_R]->u.ad.value)) * PF_RAD_PER_DEGREE;
	master.angle_b = (double)(FIX2D(params[PARAM_ANGLE_G]->u.ad.value)) * PF_RAD_PER_DEGREE;
	master.angle_g = (double)(FIX2D(params[PARAM_ANGLE_B]->u.ad.value)) * PF_RAD_PER_DEGREE;
	master.use_greyscale = (int)(params[PARAM_USE_GREYSCALE]->u.bd.value);

	// get input layer & set sample region
	PF_EffectWorld *inputLayer = &params[INPUT_LAYER]->u.ld;
	master.world = &params[INPUT_LAYER]->u.ld;
	master.in_data = in_data;
	master.samp_pb.src = &params[INPUT_LAYER]->u.ld;
	master.samp_pb.x_radius = D2FIX(master.radius_max);
	master.samp_pb.y_radius = D2FIX(master.radius_max);

	if (PF_WORLD_IS_DEEP(output)) {
		ERR(
			suites.Iterate16Suite1()->iterate(
				in_data,
				0,
				linesL,
				inputLayer,
				NULL,
				(void*)&master,
				Halftone16,
				output
			)
		);
	} else {
		ERR(
			suites.Iterate8Suite1()->iterate(
				in_data,
				0,
				linesL,
				inputLayer,
				NULL,
				(void*)&master,
				Halftone8,
				output
			)
		);
	}

	return err;
}

DllExport
PF_Err
EntryPointFunc (
	PF_Cmd cmd,
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output,
	void *extra
){
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
	} catch(PF_Err &thrown_err) {
		err = thrown_err;
	}

	return err;
}
