#include "skopin/ware.hpp"
#include "skopin/measures.hpp"
#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()){
        return "empty mark";
    }
    for (int i=0; i<ware.mark.length(); i++) {
        if (ware.mark[i]==' ') {
            return "bad mark";
        }
        if (ware.mark[i]=='\t') {
            return "bad mark";
        }
        if (ware.mark[i]=='\n') {
            return "bad mark";
        }
    }
    if (ware.height_cm<=0 or ware.height_cm>kKilnHeightCm){
        return "bad height";
    }
    if (ware.thickness_mm<=0){
        return "bad thickness";
    }
    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int total=next.height_cm;
    for (const Ware& ware: stack){
        total+=ware.height_cm+kShelfCm;
    }
    return total<=kKilnHeightCm;
}

}  // namespace skopin