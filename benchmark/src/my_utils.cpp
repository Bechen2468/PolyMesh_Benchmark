#include "my_utils.h"
#include "QuickHull.hpp"

namespace my_utils {
    namespace off {
        void build_sphere_off(size_t Lat_Count, size_t Lon_Count, const std::string& File_Path) {
            const double radius = 1.0;

            // Precompute number of vertices and faces
            size_t num_vertices = (Lat_Count + 1) * (Lon_Count + 1);
            size_t num_faces = 2 * Lat_Count * Lon_Count; // two triangles per quad

            // Reserve vectors
            std::vector<std::array<double,3>> vertices;
            vertices.reserve(num_vertices);

            std::vector<std::array<int,3>> faces;
            faces.reserve(num_faces);

            // Generate vertices
            for (size_t i = 0; i <= Lat_Count; ++i) {
                double theta = i * M_PI / Lat_Count; // latitude
                for (size_t j = 0; j <= Lon_Count; ++j) {
                    double phi = j * 2.0 * M_PI / Lon_Count; // longitude

                    double x = radius * sin(theta) * cos(phi);
                    double z = radius * sin(theta) * sin(phi);
                    double y = radius * cos(theta);

                    vertices.push_back({x, y, z});
                }
            }

            // Generate faces (two triangles per quad)
            for (size_t i = 0; i < Lat_Count; ++i) {
                for (size_t j = 0; j < Lon_Count; ++j) {
                    size_t current = i * (Lon_Count + 1) + j;
                    size_t next = current + Lon_Count + 1;

                    if (i != 0)
                        faces.push_back({int(current), int(next), int(next + 1)});

                    if (i != Lat_Count - 1)
                        faces.push_back({int(current), int(next + 1), int(current + 1)});
                }
            }

            // Write to OFF file
            std::ofstream out(File_Path);
            if (!out.is_open()) {
                std::cerr << "ERROR: Could not open file " << File_Path << "\n";
                return;
            }

            out << "OFF\n";
            out << vertices.size() << " " << faces.size() << " 0\n";

            for (auto& v : vertices)
                out << v[0] << " " << v[1] << " " << v[2] << "\n";

            for (auto& f : faces)
                out << "3 " << f[0] << " " << f[1] << " " << f[2] << "\n";

            out.close();
        }
        

        void build_random_sphere_off(size_t Point_Count, const std::string& File_Path, unsigned int Seed) {
            std::mt19937 rng(Seed);
            std::normal_distribution<double> dist(0.0, 1.0);

            std::vector<quickhull::Vector3<double>> qh_points;
            qh_points.reserve(Point_Count);

            for(size_t i = 0; i < Point_Count; i++) {
                double x = dist(rng), y = dist(rng), z = dist(rng);
                double len = std::sqrt(x*x + y*y + z*z);
                qh_points.push_back({x / len, y / len, z / len});
            }

            quickhull::QuickHull<double> qh;
            // Keep 'false' here to generate a compacted vertex buffer
            auto hull = qh.getConvexHull(qh_points, true, false); 
            
            // Get both the optimized vertices and their index buffer
            const auto& vertices = hull.getVertexBuffer(); // <--- Get the actual hull vertices
            const auto& indices = hull.getIndexBuffer();

            std::ofstream out(File_Path);
            if (!out) return;

            out << "OFF\n";
            out << vertices.size() << " " << (indices.size() / 3) << " 0\n";
            
            // Write the actual vertices returned by QuickHull
            for(auto& v : vertices) {
                out << v.x << " " << v.y << " " << v.z << "\n";
            }
            
            // Write the triangles
            for(size_t i = 0; i < indices.size(); i += 3) {
                out << "3 " << indices[i] << " " << indices[i+1] << " " << indices[i+2] << "\n";
            }
            out.close();
        }



        void build_quad_off(size_t Point_Count, const std::string& File_Path) {
            pmp::SurfaceMesh mesh = pmp::torus(std::sqrt(Point_Count), std::sqrt(Point_Count), 2.0, 0.5);
            pmp::write(mesh, File_Path);
        }


        void build_dual_off(size_t Point_Count, const std::string& File_Path, unsigned int Seed) {
            std::mt19937 rng(Seed);
            std::normal_distribution<double> dist(0.0, 1.0);
            size_t Point_Count_Half = Point_Count / 2;

            std::vector<quickhull::Vector3<double>> qh_points;
            qh_points.reserve(Point_Count_Half);

            for(size_t i = 0; i < Point_Count_Half; i++) {
                double x = dist(rng), y = dist(rng), z = dist(rng);
                double len = std::sqrt(x*x + y*y + z*z);
                qh_points.push_back({x / len, y / len, z / len});
            }

            quickhull::QuickHull<double> qh;
            auto hull = qh.getConvexHull(qh_points, true, false); 
            
            const auto& qh_vertices = hull.getVertexBuffer(); 
            const auto& qh_indices = hull.getIndexBuffer();

            // 2. Load QuickHull output directly into a PMP Triangle Mesh
            pmp::SurfaceMesh tri_mesh;
            std::vector<pmp::Vertex> pmp_verts;
            pmp_verts.reserve(qh_vertices.size());

            for (const auto& v : qh_vertices) {
                pmp_verts.push_back(tri_mesh.add_vertex(pmp::Point(v.x, v.y, v.z)));
            }

            for (size_t i = 0; i < qh_indices.size(); i += 3) {
                tri_mesh.add_face({
                    pmp_verts[qh_indices[i]],
                    pmp_verts[qh_indices[i+1]],
                    pmp_verts[qh_indices[i+2]]
                });
            }

            // 3. Generate the Dual Polymesh
            pmp::SurfaceMesh dual_mesh;
            auto dual_vertices = tri_mesh.add_face_property<pmp::Vertex>("f:dual_vertex");
            
            // Convert faces to vertices
            for (auto f : tri_mesh.faces()) {
                pmp::Point centroid(0, 0, 0);
                int count = 0;
                for (auto v : tri_mesh.vertices(f)) {
                    centroid += tri_mesh.position(v);
                    count++;
                }
                centroid /= static_cast<float>(count);
                dual_vertices[f] = dual_mesh.add_vertex(centroid);
            }

            // Convert vertices to multi-sided faces
            for (auto v : tri_mesh.vertices()) {
                if (tri_mesh.is_boundary(v)) continue;

                std::vector<pmp::Vertex> face_vertices;
                for (auto f : tri_mesh.faces(v)) {
                    face_vertices.push_back(dual_vertices[f]);
                }
                
                // This creates clean n-gons mapping perfectly to the vertex valencies!
                dual_mesh.add_face(face_vertices);
            }

            pmp::write(dual_mesh, File_Path);
        }

    };


    namespace system {
        size_t get_current_memory_kb() {
            std::ifstream status("/proc/self/status");
            std::string line;
            while(std::getline(status, line)) {
                if(line.rfind("VmRSS:", 0) == 0) {
                    size_t kb = 0;
                    sscanf(line.c_str(), "VmRSS: %zu kB", &kb);
                    return kb;
                }
            }
            return 0;
        }


        size_t get_peak_memory_kb() {
            std::ifstream status("/proc/self/status");
            std::string line;
            while(std::getline(status, line)) {
                if(line.rfind("VmHWM:", 0) == 0) {
                    size_t kb = 0;
                    sscanf(line.c_str(), "VmHWM: %zu kB", &kb);
                    return kb;
                }
            }
            return 0;
        }

    };


    namespace num {
        std::vector<size_t> generate_random_indices(size_t Size) {
            std::vector<size_t> vec(Size, 0);
            std::iota(vec.begin(), vec.end(), 0);
            
            std::random_device rd;
            std::mt19937 g(rd());
            std:shuffle(vec.begin(), vec.end(), g);
            return vec;
        }
    }
};