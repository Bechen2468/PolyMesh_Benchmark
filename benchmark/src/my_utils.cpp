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

            // uniform random points on unit sphere
            std::vector<quickhull::Vector3<double>> qh_points;
            std::vector<std::array<double,3>> vertices;
            qh_points.reserve(Point_Count);
            vertices.reserve(Point_Count);

            for(size_t i = 0; i < Point_Count; i++) {
                double x = dist(rng), y = dist(rng), z = dist(rng);
                double len = std::sqrt(x*x + y*y + z*z);
                x /= len; y /= len; z /= len;
                qh_points.push_back({x, y, z});
                vertices.push_back({x, y, z});
            }

            // convex hull on sphere = delaunay triangulation
            quickhull::QuickHull<double> qh;
            auto hull = qh.getConvexHull(qh_points, true, false);
            auto& indices = hull.getIndexBuffer();

            // write OFF
            std::ofstream out(File_Path);
            out << "OFF\n";
            out << vertices.size() << " " << (indices.size() / 3) << " 0\n";
            for(auto& v : vertices)
                out << v[0] << " " << v[1] << " " << v[2] << "\n";
            for(size_t i = 0; i < indices.size(); i += 3)
                out << "3 " << indices[i] << " " << indices[i+1] << " " << indices[i+2] << "\n";
            out.close();
        }
    };


    namespace system {
        size_t get_memory_usage() {
#if defined(_WIN32)
            PROCESS_MEMORY_COUNTERS info;
            GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
            return static_cast<size_t>(info.WorkingSetSize) / 1024;
#elif defined(__linux__)

            // TEMP TEST
            std::ifstream file("/proc/self/status");
            std::string line;
            while (std::getline(file, line)) {
                if (line.substr(0, 6) == "VmRSS:") {
                    // Extract digits
                    size_t pos = line.find_first_of("0123456789");
                    if (pos != std::string::npos) {
                        return std::stoul(line.substr(pos));
                    }
                }
            }
            return 0;
            // TEMP TEST

            std::ifstream statm("/proc/self/statm");
            size_t size, resident;
            statm >> size >> resident;
            return resident * sysconf(_SC_PAGESIZE) / 1024;
#endif
            return 0;
        }
    
    
        size_t get_resident_memory_kb() {
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