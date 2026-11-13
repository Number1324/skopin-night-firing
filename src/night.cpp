#include "skopin/night.hpp"
#include "skopin/measures.hpp"
#include "skopin/kiln.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace skopin {

namespace {

std::vector<Ware> takeBatch(std::vector<Ware>& queue) {
    std::vector<Ware> batch{queue.front()};
    std::vector<bool> taken(queue.size(), false);
    taken[0] = true;

    if (queue.front().kind != WareKind::Sagger){
        const Program program = queue.front().program;
        bool skipped = false;
        for (std::size_t i = 1; i < queue.size(); ++i){
            const Ware& ware = queue[i];

            if (ware.program != program or ware.kind == WareKind::Sagger){
                break;
            }
            if (fitsInKiln(batch, ware) and (!skipped or ware.kind == WareKind::Rush)){
                batch.push_back(ware);
                taken[i] = true;
            } else {
                skipped = true;
            }
        }
    }

    std::vector<Ware> rest;
    for (std::size_t i = 0; i < queue.size(); ++i){
        if (!taken[i]){
            rest.push_back(queue[i]);
        }
    }
    queue = std::move(rest);
    return batch;
}

void loadBatch(Kiln& kiln, const std::vector<Ware>& batch){
    for (std::size_t i = batch.size(); i > 0; --i){
        kiln.load(batch[i - 1]);
    }
}

void unloadBatch(Kiln& kiln, const std::vector<Ware>& batch,
                 std::vector<std::string>& passed){
    for (const Ware& ware : batch){
        kiln.unload(ware.mark);
        passed.push_back(ware.mark);
    }
}

void runBisque(Kiln& kiln, const std::vector<Ware>& batch, bool nextIsGlaze, std::vector<std::string>& passed){
    if (kiln.door() == Door::Open){
        kiln.close();
    }
    if (kiln.cone() == Cone::Bisque){
        kiln.cool();
    }

    kiln.open();
    loadBatch(kiln, batch);
    kiln.close();
    kiln.fire();
    kiln.open();
    unloadBatch(kiln, batch, passed);

    if (nextIsGlaze){
        return;
    }
    kiln.close();
    kiln.cool();
}

void runGlaze(Kiln& kiln, const std::vector<Ware>& batch, std::vector<std::string>& passed){
    if (kiln.door() == Door::Closed) {
        kiln.open();
    }
    loadBatch(kiln, batch);
    kiln.close();

    while (kiln.cone() != Cone::Glaze){
        kiln.fire();
    }
    kiln.cool();
    kiln.cool();
    kiln.open();
    unloadBatch(kiln, batch, passed);
    kiln.close();
}

}

NightReport runNightFire(const std::vector<Ware>& queue){
    NightReport report;

    std::vector<Ware> waiting;
    std::unordered_set<std::string> seen;
    for (const Ware& ware : queue) {
        if (!seen.insert(ware.mark).second){
            report.refused.push_back({ware.mark, "duplicate"});
        } else if (validateWare(ware)){
            report.refused.push_back({ware.mark, "invalid"});
        } else if (ware.thickness_mm > kCrackThicknessMm){
            report.refused.push_back({ware.mark, "thick"});
        } else {
            waiting.push_back(ware);
        }
    }

    std::vector<std::vector<Ware>> batches;
    while (!waiting.empty()){
        batches.push_back(takeBatch(waiting));
    }

    Kiln kiln;
    for (std::size_t i = 0; i < batches.size(); ++i){
        const std::vector<Ware>& batch = batches[i];
        if (batch.front().program == Program::Bisque){
            bool nextIsGlaze = i + 1 < batches.size() and batches[i + 1].front().program == Program::Glaze;
            runBisque(kiln, batch, nextIsGlaze, report.passed);
        } else{
            runGlaze(kiln, batch, report.passed);
        }
    }

    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin