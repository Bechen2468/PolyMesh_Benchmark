#include <benchmark/benchmark.h>
#include "benchmarks/mesh/bm_pmp.h"
#include "benchmarks/mesh/bm_polymesh_V1.h"
#include "benchmarks/mesh/bm_polymesh_V2.h"
#include "benchmarks/mesh/bm_openmesh.h"




class RelativeReporter : public benchmark::ConsoleReporter {
public:
    bool ReportContext(const Context& context) override {
        return ConsoleReporter::ReportContext(context);
    }

    void ReportRuns(const std::vector<Run>& reports) override {
        std::vector<Run> mutable_reports = reports;
        for (auto& run : mutable_reports) {
            std::string group = ExtractGroup(run.benchmark_name());
            double ns = run.GetAdjustedRealTime();

            auto it = baselines_.find(group);
            if (it == baselines_.end()) {
                baselines_[group] = ns;
                indices_.push_back(group);
                run.counters["Group"] = std::find(indices_.begin(), indices_.end(), group) - indices_.begin() + 1;
                run.counters["Group%"] = 100.0;
            } else {
                run.counters["Group"] = std::find(indices_.begin(), indices_.end(), group) - indices_.begin() + 1;
                run.counters["Group%"] = round(100.0 * ns / it->second);
            }
        }
        ConsoleReporter::ReportRuns(mutable_reports);
    }

private:
    static std::string ExtractGroup(const std::string& name) {
        std::vector<std::string> parts;
        size_t start = 0, pos;
        while ((pos = name.find('/', start)) != std::string::npos) {
            parts.push_back(name.substr(start, pos - start));
            start = pos + 1;
        }
        parts.push_back(name.substr(start));

        if (parts.size() < 3) {
            return name; // unexpected format, fall back to full name
        }

        std::string case_name = parts[2]; // e.g. "Load_All", "Iterate_V_Raw"
        const std::string& size = parts[3]; // e.g. "10000"

        
        static const std::vector<std::string> merge_suffixes = {"_All", "_Loaded", "_Raw"};
        for (const auto& suffix : merge_suffixes) {
            if (case_name.size() > suffix.size() &&
                case_name.compare(case_name.size() - suffix.size(), suffix.size(), suffix) == 0) {
                case_name.erase(case_name.size() - suffix.size());
                break;
            }
        }

        return case_name + "/" + size;
    }


    std::map<std::string, double> baselines_;
    std::vector<std::string> indices_;
};



int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    RelativeReporter reporter;
    benchmark::RunSpecifiedBenchmarks(&reporter);
    benchmark::Shutdown();
}