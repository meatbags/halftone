#include "Halftone.hpp"
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
	PF_ADD_FLOAT_SLIDER("Size", 0, 128, 0, 32, 0, 16, 0, 0, 0, PARAM_RADIUS_MAX);
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
		// sample rotated grid, get dots
		double x = (double)xL;
		double y = (double)yL;
		SamplePoints16 K = SampleGrid16(SAMPLE_KEY, master->angle_k, x, y, master, &suites);
		PF_Pixel16 output = BlankPixel16(MAX_16, MAX_16, MAX_16);

		if (master->use_greyscale) {
			// write output to pixel
			WriteToOutput16(K, output);
		} else {
			// sample RGB grids
			SamplePoints16 R = SampleGrid16(SAMPLE_RED, master->angle_r, x, y, master, &suites);
			SamplePoints16 B = SampleGrid16(SAMPLE_BLUE, master->angle_b, x, y, master, &suites);
			SamplePoints16 G = SampleGrid16(SAMPLE_GREEN, master->angle_g, x, y, master, &suites);

			// write output to pixel
			output.red = 0;
			output.blue = 0;
			output.green = 0;

			if (!CheckCollisions(R, G, B, K)) {
				output.alpha = output.red = output.blue = output.green = MAX_16;
			} else {
				WriteToOutput16(R, output);
				WriteToOutput16(B, output);
				WriteToOutput16(G, output);
				A_u_short diff = MAX_16 - max(output.red, max(output.blue, output.green));
				output.red += diff;
				output.blue += diff;
				output.green += diff;
				//WriteToOutput16(K, output);
			}
		}

		// place pixel
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
	master.radius_max = (double)(params[PARAM_RADIUS_MAX]->u.fs_d.value);
	master.angle_k = (double)(FIX2D(params[PARAM_ANGLE_K]->u.ad.value)) * PF_RAD_PER_DEGREE;
	master.angle_r = (double)(FIX2D(params[PARAM_ANGLE_R]->u.ad.value)) * PF_RAD_PER_DEGREE;
	master.angle_b = (double)(FIX2D(params[PARAM_ANGLE_G]->u.ad.value)) * PF_RAD_PER_DEGREE;
	master.angle_g = (double)(FIX2D(params[PARAM_ANGLE_B]->u.ad.value)) * PF_RAD_PER_DEGREE;
	master.use_greyscale = (int)(params[PARAM_USE_GREYSCALE]->u.bd.value);

	// get input layer & set sample region
	PF_EffectWorld *inputLayer = &params[INPUT_LAYER]->u.ld;
	master.world = &params[INPUT_LAYER]->u.ld;
	master.effect_world = params[INPUT_LAYER]->u.ld;
	master.in_data = in_data;
	master.samp_pb.src = &params[INPUT_LAYER]->u.ld;
	master.samp_pb.x_radius = D2FIX(min(8, master.radius_max));
	master.samp_pb.y_radius = D2FIX(min(8, master.radius_max));

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
