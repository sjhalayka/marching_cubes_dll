// mc_dll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <vector>
using std::vector;

#include <cmath>

#include "primitives.h"
#include "quaternion_math.h"
#include "eqparse.h"
#include "marching_cubes.h"
using namespace marching_cubes;

#include <string>
#include <iostream>
using std::string;
using std::cout;
using std::endl;



#define DLLEXPORT extern "C" __declspec(dllexport)


bool write_triangles_to_binary_stereo_lithography_file(const vector<triangle> &triangles, const char *const file_name)
{
	//cout << "Triangle count: " << triangles.size() << endl;

	if (0 == triangles.size())
		return false;

	// Write to file.
	ofstream out(file_name, ios_base::binary);

	if (out.fail())
		return false;

	const size_t header_size = 80;
	vector<char> buffer(header_size, 0);
	const unsigned int num_triangles = static_cast<unsigned int>(triangles.size()); // Must be 4-byte unsigned int.
	vertex_3 normal;

	// Write blank header.
	out.write(reinterpret_cast<const char *>(&(buffer[0])), header_size);

	// Write number of triangles.
	out.write(reinterpret_cast<const char *>(&num_triangles), sizeof(unsigned int));

	// Copy everything to a single buffer.
	// We do this here because calling ofstream::write() only once PER MESH is going to 
	// send the data to disk faster than if we were to instead call ofstream::write()
	// thirteen times PER TRIANGLE.
	// Of course, the trade-off is that we are using 2x the RAM than what's absolutely required,
	// but the trade-off is often very much worth it (especially so for meshes with millions of triangles).
	//cout << "Generating normal/vertex/attribute buffer" << endl;

	// Enough bytes for twelve 4-byte floats plus one 2-byte integer, per triangle.
	const size_t data_size = (12 * sizeof(float) + sizeof(short unsigned int)) * num_triangles;
	buffer.resize(data_size, 0);

	// Use a pointer to assist with the copying.
	// Should probably use std::copy() instead, but memcpy() does the trick, so whatever...
	char *cp = &buffer[0];

	for (vector<triangle>::const_iterator i = triangles.begin(); i != triangles.end(); i++)
	{
		// Get face normal.
		vertex_3 v0 = i->vertex[1] - i->vertex[0];
		vertex_3 v1 = i->vertex[2] - i->vertex[0];
		normal = v0.cross(v1);
		normal.normalize();

		memcpy(cp, &normal.x, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &normal.y, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &normal.z, sizeof(float)); cp += sizeof(float);

		memcpy(cp, &i->vertex[0].x, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &i->vertex[0].y, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &i->vertex[0].z, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &i->vertex[1].x, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &i->vertex[1].y, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &i->vertex[1].z, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &i->vertex[2].x, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &i->vertex[2].y, sizeof(float)); cp += sizeof(float);
		memcpy(cp, &i->vertex[2].z, sizeof(float)); cp += sizeof(float);

		cp += sizeof(short unsigned int);
	}

	//cout << "Writing " << data_size / 1048576 << " MB of data to binary Stereo Lithography file: " << file_name << endl;

	out.write(reinterpret_cast<const char *>(&buffer[0]), data_size);
	out.close();

	return true;
}



DLLEXPORT unsigned int get_normal(const char *equation_text,
							float z_w,
							float C_x, float C_y, float C_z, float C_w,
							unsigned int max_iterations,
							float threshold,
							float x_grid_min, float x_grid_max, 
							float y_grid_min, float y_grid_max, 
							float z_grid_min, float z_grid_max, 
							unsigned int x_res, unsigned int y_res, unsigned int z_res,
							float *normal_output,
							unsigned int make_border,
							unsigned int write_triangles)
{

	//cout << z_w << endl;
	//cout << C_x << " " << C_y << " " << C_z << " " << C_w << endl;
	//cout << max_iterations << endl;
	//cout << threshold << endl;
	//cout << x_grid_min << " " << x_grid_max << endl;
	//cout << y_grid_min << " " << y_grid_max << endl;
	//cout << z_grid_min << " " << z_grid_max << endl;
	//cout << x_res << " " << y_res << " " << z_res << endl;
	//cout << make_border << endl;
	//cout << write_triangles << endl;
	//cout << endl << endl;


	vector<triangle> triangles;
	quaternion C;
	C.x = C_x;
	C.y = C_y;
	C.z = C_z;
	C.w = C_w;

	string error_string;
	quaternion_julia_set_equation_parser eqparser;
	if (false == eqparser.setup(equation_text, error_string, C))
	{
		cout << "Equation error: " << error_string << endl;
		return 1;
	}

	// When adding a border, use a value that is "much" greater than the threshold.
	const float border_value = 1.0f + threshold;

	//vector<triangle> triangles;
	vector<float> xyplane0(x_res*y_res, 0);
	vector<float> xyplane1(x_res*y_res, 0);

	const float x_step_size = (x_grid_max - x_grid_min) / (x_res - 1);
	const float y_step_size = (y_grid_max - y_grid_min) / (y_res - 1);
	const float z_step_size = (z_grid_max - z_grid_min) / (z_res - 1);

	size_t z = 0;

	quaternion Z(x_grid_min, y_grid_min, z_grid_min, z_w);

	// Calculate 0th xy plane.
	for (size_t x = 0; x < x_res; x++, Z.x += x_step_size)
	{
		Z.y = y_grid_min;

		for (size_t y = 0; y < y_res; y++, Z.y += y_step_size)
		{
			if (1 == make_border && (x == 0 || y == 0 || z == 0 || x == x_res - 1 || y == y_res - 1 || z == z_res - 1))
				xyplane0[x*y_res + y] = border_value;
			else
				xyplane0[x*y_res + y] = eqparser.iterate(Z, max_iterations, threshold);
		}
	}

	// Prepare for 1st xy plane.
	z++;
	Z.z += z_step_size;

	// Calculate 1st and subsequent xy planes.
	for (; z < z_res; z++, Z.z += z_step_size)
	{
		Z.x = x_grid_min;

		//cout << "Calculating triangles from xy-plane pair " << z << " of " << z_res - 1 << endl;

		for (size_t x = 0; x < x_res; x++, Z.x += x_step_size)
		{
			Z.y = y_grid_min;

			for (size_t y = 0; y < y_res; y++, Z.y += y_step_size)
			{
				if (1 == make_border && (x == 0 || y == 0 || z == 0 || x == x_res - 1 || y == y_res - 1 || z == z_res - 1))
					xyplane1[x*y_res + y] = border_value;
				else
					xyplane1[x*y_res + y] = eqparser.iterate(Z, max_iterations, threshold);
			}
		}

		// Calculate triangles for the xy-planes corresponding to z - 1 and z by marching cubes.
		tesselate_adjacent_xy_plane_pair(
			xyplane0, xyplane1,
			z - 1,
			triangles,
			threshold, // Use threshold as isovalue.
			x_grid_min, x_grid_max, x_res,
			y_grid_min, y_grid_max, y_res,
			z_grid_min, z_grid_max, z_res);

		// Swap memory pointers (fast) instead of performing a memory copy (slow).
		xyplane1.swap(xyplane0);
	}

	//cout << endl;

	if (triangles.size() == 0)
	{
		//cout << "no triangles????" << endl;
		return 0;
	}

	if (1 == write_triangles)
	{
		cout << "writing " << triangles.size() << endl;
		write_triangles_to_binary_stereo_lithography_file(triangles, "out.stl");
	}

	float nx = 0, ny = 0, nz = 0;

	for (size_t i = 0; i < triangles.size(); i++)
	{
		vertex_3 v = triangles[i].get_normal();
		nx += v.x;
		ny += v.y;
		nz += v.z;
	}

	float n_len = sqrt(nx*nx + ny*ny + nz*nz);

	if (0 != n_len)
	{
		nx /= n_len;
		ny /= n_len;
		nz /= n_len;
	}

	normal_output[0] = nx;
	normal_output[1] = ny;
	normal_output[2] = nz;

	return 0;
}