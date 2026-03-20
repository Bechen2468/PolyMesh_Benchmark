#include "my_utils.h"


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
            std::cout << "Sphere written to " << File_Path
                    << " with " << vertices.size() << " vertices and "
                    << faces.size() << " faces.\n";
        }
        
    };


    namespace system {
        size_t get_memory_usage() {
#if defined(_WIN32)
            PROCESS_MEMORY_COUNTERS info;
            GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
            return static_cast<size_t>(info.WorkingSetSize) / 1024;
#elif defined(__linux__)
            std::ifstream statm("/proc/self/statm");
            size_t size, resident;
            statm >> size >> resident;
            return resident * sysconf(_SC_PAGESIZE) / 1024;
#endif
            return 0;
        }
    };
};