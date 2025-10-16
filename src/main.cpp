#include "benchmark_result/benchmark_result_storage.h"
#include "benchmark/benchmark_openmesh.h"
#include "benchmark/benchmark_polymesh.h"
#include "my_utils.h"



int main() {
    Benchmark_Result_Storage storage;
    std::string off_path = "./sphere.off";
    std::vector<Benchmark_Result> results;

    for(int i = 2; i < 10; ++i) {
        size_t complexity = std::pow(2, i);
        std::cout << "Building Sphere with complexity "  << complexity << "." << std::endl;
        my_utils::off::build_sphere_file(complexity, complexity, off_path);

        Benchmark_PolyMesh bm_poly(off_path, complexity);
        Benchmark_OpenMesh bm_open(off_path, complexity);

        std::cout << "Starting benchmark.barycenter with complexity "  << complexity << "." << std::endl;
        results.push_back(bm_poly.test_barycenter());
        results.push_back(bm_open.test_barycenter());
    }

    storage.write_results(results);

    std::cout << "Benchmark finished...";
    getchar();
    return 0;
}
