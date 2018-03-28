#pragma once
#ifndef HALFTONE_SAMPLER_H
#define HALFTONE_SAMPLER_H

struct Sampler {
	Vector normal = Vector(0, 0);
	double angle;
	Vector p1 = Vector(0, 0);
	Vector p2 = Vector(0, 0);
	Vector p3 = Vector(0, 0);
	Vector p4 = Vector(0, 0);
	PF_Pixel8 sample1 = PF_Pixel8();
	PF_Pixel8 sample2 = PF_Pixel8();
	PF_Pixel8 sample3 = PF_Pixel8();
	PF_Pixel8 sample4 = PF_Pixel8();

	Sampler(double x, double y, Vector n) {
		p1.x = x;
		p1.y = y;
		normal.x = n.x;
		normal.y = n.y;
		angle = atan2(normal.y, normal.x);
	}

	void writeChannel8(
		A_u_char *out,
		A_u_char sample,
		Vector point,
		Vector dot,
		A_u_char mode,
		double radius_max,
		double aa
	) {
		double scale = 1.0 - sample / 255.0;
		scale = scale + scale - (3 * pow(scale, 2) - 2 * pow(scale, 3));

		if (mode > 1) {
			double theta = atan2(dot.y - point.y, dot.x - point.x);

			if (mode == 2) {
				// square
				scale += abs(SQRT2_HALF_MINUS_ONE * sin(2 * theta) * scale);
			} else if (mode == 3) {
				// euclidean dot
				double r2 = scale + abs(SQRT2_MINUS_ONE * sin(2 * theta) * scale);
				scale = BLEND(scale, r2, scale);
			} else if (mode == 4) {
				// ellipse
				scale *= 1.414 * sin(theta + angle);
			} else if (mode == 5) {
				// line
				scale *= 1.414 * sin(theta + angle);
			} else if (mode == 6) {
				// diamond
				scale -= abs(SQRT2_HALF_MINUS_ONE * sin(2 * theta) * scale);
			} else {
				// flower
				scale *= SQRT2 * abs(sin(2 * theta));
			}
		}

		double radius = radius_max * scale;
		double distance = point.getDistanceTo(&dot);

		if (distance < radius) {
			*out = (A_u_char)max(0.0, (*out - (CLAMP(0.0, 1.0, (radius - distance) / aa) * 255)));
		}
	}

	PF_Err write8(
		int channel,
		A_u_char *ch,
		Vector point,
		A_u_char mode,
		double radius,
		double aa
	) {
		PF_Err err = PF_Err_NONE;

		if (channel == 1) {
			writeChannel8(ch, sample1.red, point, p1, mode, radius, aa);
			writeChannel8(ch, sample2.red, point, p2, mode, radius, aa);
			writeChannel8(ch, sample3.red, point, p3, mode, radius, aa);
			writeChannel8(ch, sample4.red, point, p4, mode, radius, aa);
		} else if (channel == 2) {
			writeChannel8(ch, sample1.green, point, p1, mode, radius, aa);
			writeChannel8(ch, sample2.green, point, p2, mode, radius, aa);
			writeChannel8(ch, sample3.green, point, p3, mode, radius, aa);
			writeChannel8(ch, sample4.green, point, p4, mode, radius, aa);
		} else {
			writeChannel8(ch, sample1.blue, point, p1, mode, radius, aa);
			writeChannel8(ch, sample2.blue, point, p2, mode, radius, aa);
			writeChannel8(ch, sample3.blue, point, p3, mode, radius, aa);
			writeChannel8(ch, sample4.blue, point, p4, mode, radius, aa);
		}

		return err;
	}

	PF_Err sample(
		PF_Sampling8Suite1 *sampling_suite,
		PF_ProgPtr effect_ref,
		HalftoneInfo *info
	) {
		// sample image at gridpoints
		PF_Err err = PF_Err_NONE;
		double width = info->samp_pb.src->width - 1;
		double height = info->samp_pb.src->height - 1;
		
		Vector s1 = getGridPoint(p1.x, p1.y, info->origin, info->normal_0, info->grid_step, info->grid_half_step);
		s1.clamp(width, height);
		ERR(sampling_suite->nn_sample(effect_ref, D2FIX(s1.x), D2FIX(s1.y), &info->samp_pb, &sample1));
		
		Vector s2 = getGridPoint(p2.x, p2.y, info->origin, info->normal_0, info->grid_step, info->grid_half_step);
		s2.clamp(width, height);
		ERR(sampling_suite->nn_sample(effect_ref, D2FIX(s2.x), D2FIX(s2.y), &info->samp_pb, &sample2));
		
		Vector s3 = getGridPoint(p3.x, p3.y, info->origin, info->normal_0, info->grid_step, info->grid_half_step);
		s3.clamp(width, height);
		ERR(sampling_suite->nn_sample(effect_ref, D2FIX(s3.x), D2FIX(s3.y), &info->samp_pb, &sample3));
		
		Vector s4 = getGridPoint(p4.x, p4.y, info->origin, info->normal_0, info->grid_step, info->grid_half_step);
		s4.clamp(width, height);
		ERR(sampling_suite->nn_sample(effect_ref, D2FIX(s4.x), D2FIX(s4.y), &info->samp_pb, &sample4));

		return err;
	}
};

Sampler getSampler(double x, double y, Vector origin, Vector normal, double step, double half_step) {
	// project point onto rotated grid, get containing cell
	double dot_normal = normal.x * (x - origin.x) + normal.y * (y - origin.y);
	double dot_line = -normal.y * (x - origin.x) + normal.x * (y - origin.y);
	double offset_x = normal.x * dot_normal;
	double offset_y = normal.y * dot_normal;
	double offset_normal = fmod(hypot(offset_x, offset_y), step);
	double normal_dir = (dot_normal < 0) ? 1 : -1;
	double normal_scale = ((offset_normal < half_step) ? -offset_normal : step - offset_normal) * normal_dir;
	double offset_line = fmod(hypot((x - offset_x) - origin.x, (y - offset_y) - origin.y), step);
	double line_dir = (dot_line < 0) ? 1 : -1;
	double line_scale = ((offset_line < half_step) ? -offset_line : step - offset_line) * line_dir;

	// set first point to nearest gridpoint
	Sampler res = Sampler(
		x - normal.x * normal_scale + normal.y * line_scale,
		y - normal.y * normal_scale - normal.x * line_scale,
		normal
	);

	// calculate cell corners
	double normal_step = normal_dir * ((offset_normal < half_step) ? step : -step);
	double line_step = line_dir * ((offset_line < half_step) ? step : -step);
	res.p2.x = res.p1.x - normal.x * normal_step;
	res.p2.y = res.p1.y - normal.y * normal_step;
	res.p3.x = res.p1.x + normal.y * line_step;
	res.p3.y = res.p1.y - normal.x * line_step;
	res.p4.x = res.p1.x - normal.x * normal_step + normal.y * line_step;
	res.p4.y = res.p1.y - normal.y * normal_step - normal.x * line_step;

	return res;
}

#endif