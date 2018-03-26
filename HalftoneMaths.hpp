#pragma once
#ifndef HALFTONE_MATHS_H
#define HALFTONE_MATHS_H

// PF_Fixed = long, PF_FpLong = double
#define D2FIX(x) (((long)x)<<16)
#define FIX2D(x) (x / ((double)(1L << 16)))
#define PI 3.14159265
#define HALF_PI 1.57079632
#define CLAMP(mn, mx, x) (max(mn, min(mx, x)))

struct Vector {
	double x;
	double y;

	Vector(double x_start, double y_start) : x(x_start), y(y_start) {}
	
	void set(double x_set, double y_set) {
		x = x_set;
		y = y_set;
	}

	double getMagnitude() {
		return hypot(x, y);
	}

	double getDistanceTo(Vector *p) {
		return hypot(p->x - x, p->y - y);
	}

	void clamp(double x_max, double y_max) {
		x = CLAMP(0.0, x_max, x);
		y = CLAMP(0.0, y_max, y);
	}
};

struct SampleCell {
	Vector p1 = Vector(0, 0);
	Vector p2 = Vector(0, 0);
	Vector p3 = Vector(0, 0);
	Vector p4 = Vector(0, 0);
	Vector s1 = Vector(0, 0);
	Vector s2 = Vector(0, 0);
	Vector s3 = Vector(0, 0);
	Vector s4 = Vector(0, 0);
	PF_Pixel8 sample1 = PF_Pixel8();
	PF_Pixel8 sample2 = PF_Pixel8();
	PF_Pixel8 sample3 = PF_Pixel8();
	PF_Pixel8 sample4 = PF_Pixel8();

	SampleCell(double x, double y) {
		p1.x = x;
		p1.y = y;
	}

	void clamp(double max_x, double max_y) {
		// clamp points
		s1.x = CLAMP(0.0, max_x, p1.x);
		s1.y = CLAMP(0.0, max_y, p1.y);
		s2.x = CLAMP(0.0, max_x, p2.x);
		s2.y = CLAMP(0.0, max_y, p2.y);
		s3.x = CLAMP(0.0, max_x, p3.x);
		s3.y = CLAMP(0.0, max_y, p3.y);
		s4.x = CLAMP(0.0, max_x, p4.x);
		s4.y = CLAMP(0.0, max_y, p4.y);
	}
};

Vector getGridPoint(double x, double y, Vector origin, Vector normal, double step, double half_step) {
	// project point onto rotated grid, get nearest gridpoint
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

	// set nearest gridpoint in new vector
	return Vector(
		x - normal.x * normal_scale + normal.y * line_scale,
		y - normal.y * normal_scale - normal.x * line_scale
	);
}

SampleCell getCell(double x, double y, Vector origin, Vector normal, double step, double half_step) {
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
	SampleCell res = SampleCell(
		x - normal.x * normal_scale + normal.y * line_scale,
		y - normal.y * normal_scale - normal.x * line_scale
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

#endif // HALFTONE_MATHS_H