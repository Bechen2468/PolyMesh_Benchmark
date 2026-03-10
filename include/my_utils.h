#ifndef MY_UTILS_H
#define MY_UTILS_H
// ----------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
// ----------------------------------------------------------------------------
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
// ----------------------------------------------------------------------------
#include "benchmark/result/benchmark_result.h"
// ----------------------------------------------------------------------------
typedef OpenMesh::PolyMesh_ArrayKernelT<>  Mesh;



namespace my_utils {
    namespace off {
        [[deprecated]]
        Mesh build_sphere(Mesh& m, const size_t& Lat_Count, const size_t& Lon_Count) {
            const double radius = 1.0f;

            std::vector<OpenMesh::VertexHandle> vhandle;
            vhandle.reserve((Lat_Count + 1) * (Lon_Count + 1));

            // create vertices
            for (size_t i = 0; i <= Lat_Count; ++i) {
                double theta = static_cast<double>(i) * M_PI / Lat_Count;  // Latitude angle
                //float v = static_cast<float>(i) / (float)lat;
                
                for (size_t j = 0; j <= Lon_Count; ++j) {
                    double phi = static_cast<double>(j) * 2.0f * M_PI / Lon_Count;  // Longitude angle
                    //float u = static_cast<float>(j) / (float)lon;

                    // Calculate vertex position using spherical coordinates
                    double x = radius * sinf(theta) * cosf(phi);
                    double z = radius * sinf(theta) * sinf(phi);
                    double y = radius * cosf(theta);

                    
                    vhandle.push_back(m.add_vertex(Mesh::Point(x, y, z)));

                    // TEMP OLD
                    //Vector4 pos(x, y, z, 1);
                    //Vector3 normal = Vector3(x, y, z).normalize();
                    //Vertex vertex = Vertex(pos, normal, u, v);

                    // Add the vertex to the list
                    //vertices.push_back(vertex);
                }
            }

            // create faces
            for (size_t i = 0; i < Lat_Count; ++i) {
                for (size_t j = 0; j < Lon_Count; ++j) {
                    size_t current = i * (Lon_Count + 1) + j;
                    size_t next = current + Lon_Count + 1;

                    //Build face1
                    if (i != 0) {   //skip if at pole
                        m.add_face({
                            vhandle[current],
                            vhandle[next],
                            vhandle[next + 1]
                        });
                    }

                    //Build face2
                    if (i != Lat_Count - 1) {   //skip if at pole
                        m.add_face({
                            vhandle[current],
                            vhandle[next + 1],
                            vhandle[current + 1]
                        });
                    }
                }
            }
            return m;
        }
        

        void build_sphere_off(size_t Lat_Count, size_t Lon_Count, const std::string& File_Path = "sphere.off") {
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
            std::cout << "Sphere written to " << File_Path
                    << " with " << vertices.size() << " vertices and "
                    << faces.size() << " faces.\n";
        }
        
    };


};

#endif
