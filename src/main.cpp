#include "my_utils.h"
#include "benchmark/result/benchmark_result_storage.h"
#include "benchmark/benchmark_openmesh.h"
#include "benchmark/benchmark_polymesh.h"



int main() {
    Benchmark_Result_Storage storage;
    std::string off_path = "./sphere.off";

    for(size_t i = 5; i <= 8; i += 1) {
        std::vector<Benchmark_Result> results;

        size_t vertices_count = pow(10, i);
        size_t row_count = sqrt(vertices_count);

        std::cout << "Building Sphere with complexity " << i << ", (=" << vertices_count << " Vertices)." << std::endl;
        my_utils::off::build_sphere_off(row_count, row_count, off_path);


        std::unique_ptr<Benchmark_OpenMesh> bm_open;
        std::unique_ptr<Benchmark_PolyMesh> bm_poly;


        std::cout << "Starting openmesh.run_iterate_V with complexity "  << i << "." << std::endl;
        bm_open = std::make_unique<Benchmark_OpenMesh>(off_path, i);
        results.push_back(bm_open->run_iterate_V());
        bm_open.reset();

        std::cout << "Starting polymesh.run_iterate_V with complexity "  << i << "." << std::endl;
        bm_poly = std::make_unique<Benchmark_PolyMesh>(off_path, i);
        results.push_back(bm_poly->run_iterate_V());
        bm_poly.reset();
        
        
        std::cout << "Starting openmesh.run_iterate_F with complexity "  << i << "." << std::endl;
        bm_open = std::make_unique<Benchmark_OpenMesh>(off_path, i);
        results.push_back(bm_open->run_iterate_F());
        bm_open.reset();

        std::cout << "Starting polymesh.run_iterate_F with complexity "  << i << "." << std::endl;
        bm_poly = std::make_unique<Benchmark_PolyMesh>(off_path, i);
        results.push_back(bm_poly->run_iterate_F());
        bm_poly.reset();
        

        std::cout << "Starting openmesh.run_iterate_E with complexity "  << i << "." << std::endl;
        bm_open = std::make_unique<Benchmark_OpenMesh>(off_path, i);
        results.push_back(bm_open->run_iterate_E());
        bm_open.reset();

        std::cout << "Starting polymesh.run_iterate_E with complexity "  << i << "." << std::endl;
        bm_poly = std::make_unique<Benchmark_PolyMesh>(off_path, i);
        results.push_back(bm_poly->run_iterate_E());
        bm_poly.reset();

        std::cout << "==============================================================" << std::endl;
        
        storage.write_results(results);
    }


    std::cout << "Benchmark finished...";
    getchar();
    return 0;
}
