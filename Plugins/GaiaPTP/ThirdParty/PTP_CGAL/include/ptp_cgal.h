#pragma once

#ifdef _MSC_VER
#  define PTP_CGAL_API __declspec(dllexport)
#else
#  define PTP_CGAL_API
#endif

extern "C" {

// Triangulate unit-sphere points using CGAL convex hull.
// - xyz: array of length 3*N (x,y,z triples)
// - triangles_out: output array of triplets (index into 0..N-1)
// - triangles_capacity: number of triplets capacity
// - triangles_written: returns number of triplets written
// Returns 1 on success, 0 on failure.
PTP_CGAL_API int ptp_cgal_triangulate(const double* xyz, int N,
                                      int* triangles_out, int triangles_capacity,
                                      int* triangles_written);

}

