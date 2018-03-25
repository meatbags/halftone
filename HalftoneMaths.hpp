#pragma once
#ifndef HALFTONE_MATHS_H
#define HALFTONE_MATHS_H

// PF_Fixed = long, PF_FpLong = double
#define D2FIX(x) (((long)x)<<16)
#define FIX2D(x) (x / ((double)(1L << 16)))
#define PI 3.14159265
#define HALF_PI 1.57079632

struct Vector {
	double x;
	double y;
	Vector(double xD, double yD) : x(xD), y(yD) {}
	
	Vector getProjected(Vector origin, Vector normal, double step, double half_step) {
		// find minimum offset to rotated grid
		double dotN = normal.x * (x - origin.x) + normal.y * (y - origin.y);
		double dotL = -normal.y * (x - origin.x) + normal.x * (y - origin.y);
		double off_x = normal.x * dotN;
		double off_y = normal.y * dotN;
		double off_len = fmod(hypot(off_x, off_y), step);
		double scale_norm = ((off_len < half_step) ? -off_len : step - off_len) * ((dotN < 0) ? 1 : -1);
		double off_int = fmod(hypot((x - off_x) - origin.x, (y - off_y) - origin.y), step);
		double scale_line = ((off_int < half_step) ? -off_int : step - off_int) * ((dotL < 0) ? 1 : -1);

		// apply offsets in new vector
		return Vector(
			x - normal.x * scale_norm + normal.y * scale_line,
			y - normal.y * scale_norm - normal.x * scale_line
		);
	}

	double getLength() {
		return hypot(x, y);
	}

	double getDistanceTo(Vector v) {
		return hypot(v.x - x, v.y - y);
	}

	void set(double xD, double yD) {
		x = xD;
		y = yD;
	}

	void normalise() {
		double h = hypot(x, y);
		if (h != 0.) {
			x /= h;
			y /= h;
		}
	}
};

#endif // HALFTONE_MATHS_H