#include "benchmark/result/benchmark_result_storage.h"
#include "benchmark/benchmark_openmesh.h"
#include "benchmark/benchmark_polymesh.h"
#include "my_utils.h"



int main() {
    Benchmark_Result_Storage storage;
    std::string off_path = "./sphere.off";

    for(size_t i = 2; i <= 6; i += 2) {
        std::vector<Benchmark_Result> results;

        size_t vertices_count = pow(10, i);
        size_t row_count = sqrt(vertices_count);

        std::cout << "Building Sphere with complexity " << i << ", (=" << vertices_count << " Vertices)." << std::endl;
        my_utils::off::build_sphere_off(row_count, row_count, off_path);


        std::cout << "Loading openmesh" << std::endl;
        std::unique_ptr<Benchmark_OpenMesh> bm_open = std::make_unique<Benchmark_OpenMesh>(off_path, i);
        std::cout << "Loading polymesh" << std::endl;
        std::unique_ptr<Benchmark_PolyMesh> bm_poly = std::make_unique<Benchmark_PolyMesh>(off_path, i);


        std::cout << "Starting openmesh.run_iterate_V with complexity "  << i << "." << std::endl;
        results.push_back(bm_open->run_iterate_V());
        std::cout << "Starting polymesh.run_iterate_V with complexity "  << i << "." << std::endl;
        results.push_back(bm_poly->run_iterate_V());
        
        std::cout << "Starting openmesh.run_iterate_F with complexity "  << i << "." << std::endl;
        results.push_back(bm_open->run_iterate_F());
        std::cout << "Starting polymesh.run_iterate_F with complexity "  << i << "." << std::endl;
        results.push_back(bm_poly->run_iterate_F());
        
        std::cout << "Starting openmesh.run_iterate_E with complexity "  << i << "." << std::endl;
        results.push_back(bm_open->run_iterate_E());
        std::cout << "Starting polymesh.run_iterate_E with complexity "  << i << "." << std::endl;
        results.push_back(bm_poly->run_iterate_E());

        std::cout << "==============================================================" << std::endl;
        
        storage.write_results(results);
    }


    std::cout << "Benchmark finished...";
    getchar();
    return 0;
}
