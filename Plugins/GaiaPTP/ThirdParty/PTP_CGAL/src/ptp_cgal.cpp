#include "ptp_cgal.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/convex_hull_3.h>
#include <unordered_map>
#include <vector>
#include <cmath>

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point  = Kernel::Point_3;
using Mesh   = CGAL::Surface_mesh<Point>;

struct Key { long long x,y,z; bool operator==(const Key& o) const { return x==o.x && y==o.y && z==o.z; } };
struct KeyHash { size_t operator()(const Key& k) const { return std::hash<long long>()(k.x) ^ (std::hash<long long>()(k.y)<<1) ^ (std::hash<long long>()(k.z)<<2); } };

static Key make_key(const Point& p)
{
    const double s = 1e6;
    return Key{ (long long)llround(p.x()*s), (long long)llround(p.y()*s), (long long)llround(p.z()*s) };
}

extern "C" int ptp_cgal_triangulate(const double* xyz, int N, int* tris, int tri_cap, int* tri_written)
{
    if (!xyz || !tris || !tri_written || N < 4) return 0;
    std::vector<Point> pts; pts.reserve(N);
    std::unordered_map<Key,int,KeyHash> index; index.reserve((size_t)N*2);
    for (int i=0;i<N;++i)
    {
        const double x=xyz[3*i+0], y=xyz[3*i+1], z=xyz[3*i+2];
        Point p(x,y,z);
        pts.emplace_back(p);
        index.emplace(make_key(p), i);
    }

    Mesh hull;
    CGAL::convex_hull_3(pts.begin(), pts.end(), hull);
    if (hull.number_of_faces()==0) return 0;

    int count = 0;
    for (auto f : hull.faces())
    {
        int ids[3]; int k=0;
        for (auto v : CGAL::vertices_around_face(hull.halfedge(f), hull))
        {
            const Point& p = hull.point(v);
            auto it = index.find(make_key(p));
            if (it==index.end()) return 0;
            ids[k++] = it->second;
        }
        if (k==3)
        {
            if (count >= tri_cap) break;
            tris[3*count+0]=ids[0];
            tris[3*count+1]=ids[1];
            tris[3*count+2]=ids[2];
            ++count;
        }
    }
    *tri_written = count;
    return 1;
}

