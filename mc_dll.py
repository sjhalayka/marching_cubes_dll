import ctypes as ct
import math
from utils_3d import V3


"""
DLLEXPORT int get_normal(const char *equation_text,
							float z_w,
							float C_x, float C_y, float C_z, float C_w,
							unsigned short int max_iterations,
							float threshold,
							float x_grid_min, float x_grid_max, float y_grid_min,
							float y_grid_max, float z_grid_min, float z_grid_max, 
							unsigned int x_res, unsigned int y_res, unsigned int z_res,
							float *normal_output,
							unsigned int make_border,
							unsigned int write_triangles)
							"""

def get_normal_from_mc( src_string,
                        src_z_w,
                        src_C_x, src_C_y, src_C_z, src_C_w,
			src_max_iterations,
			src_threshold,
                        src_x_grid_min, src_x_grid_max,
                        src_y_grid_min, src_y_grid_max,
                        src_z_grid_min, src_z_grid_max,
                        src_x_res, src_y_res, src_z_res,
                        src_make_border,
                        src_write_triangles):

    float3 = ct.c_float * 3
    int1 = ct.c_uint
    float1 = ct.c_float

    z_w = float1(src_z_w)
    
    C_x = float1(src_C_x)
    C_y = float1(src_C_y)
    C_z = float1(src_C_z)
    C_w = float1(src_C_w)

    max_iterations = int1(src_max_iterations)
    threshold  = float1(src_threshold)
    
    x_grid_min = float1(src_x_grid_min)
    x_grid_max = float1(src_x_grid_max)
    y_grid_min = float1(src_y_grid_min)
    y_grid_max = float1(src_y_grid_max)
    z_grid_min = float1(src_z_grid_min)
    z_grid_max = float1(src_z_grid_max)

    x_res = int1(src_x_res)
    y_res = int1(src_y_res)
    z_res = int1(src_z_res)

    output_arr = float3()

    make_border = int1(src_make_border)
    write_triangles = int1(src_write_triangles)

    lib = ct.CDLL("mc_dll.dll")

    lib.get_normal(src_string,
                    z_w,
                    C_x, C_y, C_z, C_w,
                    max_iterations,
                    threshold,
                    x_grid_min, x_grid_max,
                    y_grid_min, y_grid_max,
                    z_grid_min, z_grid_max,
                    x_res, y_res, z_res,
                    output_arr,
                    make_border,
                    write_triangles)

    ret = V3(output_arr[0], output_arr[1], output_arr[2]) 

    return ret
