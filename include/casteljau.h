#include <list>
#include <vector>
#include <algorithm>

#include "point.h"

point evaluate(float t, std::list<point> P)
{
	std::list<point> Q = P;

	while (Q.size() > 1) {
		std::list<point> R;
		std::list<point>::iterator p1;
		std::list<point>::iterator p2;

		for (p1 = Q.begin(), p2 = std::next(p1); p2 != Q.end(); ++p1, ++p2) {
			point p = ((1 - t) * *p1) + (t * *p2);

			R.push_back(p);
		}

		Q.clear();
		Q = R;
	}

	return *Q.begin();
}

std::vector<point> EvaluateBezierCurve(std::vector<point>ctrl_points, int num_evaluations)
{
	std::list<point> ps(ctrl_points.begin(), ctrl_points.end());
	std::vector<point> curve;
	float offset = 1.f / num_evaluations;
	
	curve.push_back(ctrl_points.at(0));

	for (int e = 0; e < num_evaluations; e++) {
		point p = evaluate(offset * (e + 1), ps);
		curve.push_back(p);
	}

	return curve;
}

float* MakeFloatsFromVector(std::vector<point> curve, int &num_verts, int &num_floats, float r, float g, float b)
{
	num_verts = curve.size();
	if (num_verts == 0)
		return NULL;

	num_floats = num_verts * 6;
	float* vertices = (float *) malloc(num_floats * sizeof(float));
	if (!vertices) {
		num_verts = 0;
		num_floats = 0;
		return NULL;
	}

	std::vector<point>::iterator p;
	int i = 0;

	for (p = curve.begin(); p != curve.end(); ++p) {
		// vertex coordinates
		vertices[i] = p->x;
		vertices[i + 1] = p->y;
		vertices[i + 2] = p->z;

		//vertex colour
		vertices[i + 3] = r;
		vertices[i + 4] = g;
		vertices[i + 5] = b;

		i += 6;
	}

	return vertices;
}

