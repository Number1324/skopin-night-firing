#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"
#include <stdexcept>
#include <algorithm>
#include <cstddef>

namespace skopin {

Kiln::Kiln()
    : cone_(Cone::Cold), door_(Door::Closed), stokes_(0){
}

Cone Kiln::cone() const {
    return cone_;
}

Door Kiln::door() const {
    return door_;
}

int Kiln::stokes() const {
    return stokes_;
}

std::vector<Ware> Kiln::stack() const {
    return stack_;
}

void Kiln::open() {
    if (door_ == Door::Open){
        throw KilnError("door already open");
    }
    if (cone_ == Cone::Glaze){
        throw KilnError("too hot");
    }
    door_=Door::Open;
    // Повторное открытие и открытие на поливном конусе — KilnError.
}

void Kiln::close() {
    if (door_ == Door::Closed){
        throw KilnError("door already closed");
    }
    door_=Door::Closed;
}

void Kiln::fire() {
    if (door_ != Door::Closed){
        throw KilnError("door is open");
    }
    if (cone_ == Cone::Glaze){
        throw KilnError("already at the top cone");
    }
    for (const Ware& ware: stack_){
        if (ware.thickness_mm>kCrackThicknessMm){
            throw KilnError("ware is too thick");
        }
    }
    if (cone_ == Cone::Bisque){
        for (const Ware& ware: stack_){
            if (ware.program==Program::Bisque){
                throw KilnError("bispue would crack");
            }
        }
    }
    cone_ = (cone_ == Cone::Cold) ? Cone::Bisque : Cone::Glaze;
    ++stokes_;
    if (cone_ == Cone::Glaze) {
        std::fill(glazeSeen_.begin(), glazeSeen_.end(), true);
    }
}

void Kiln::cool() {
    if (door_ != Door::Closed){
        throw KilnError("door is open");
    }
    if (cone_ == Cone::Cold){
        throw KilnError("already cold");
    }
    if (cone_ == Cone::Bisque){
        for (std::size_t i=0; i<stack_.size(); ++i){
            if (stack_[i].program == Program::Glaze and !glazeSeen_[i]){
                throw KilnError("galze were not fired");
            }
        }
    }
    cone_ = (cone_ == Cone::Glaze) ? Cone::Bisque : Cone::Cold;
    ++stokes_;
}

void Kiln::load(const Ware& ware){
    if (door_ != Door::Open){
        throw KilnError("door is closed");
    }
    bool coneOk = (cone_ == Cone::Cold) or (cone_ == Cone::Bisque and ware.program == Program::Glaze);
    if (!coneOk){
        throw KilnError("wrong cone for this ware");
    }
    if (validateWare(ware)){
        throw KilnError("invalid ware");
    }
    for (const Ware& item: stack_){
        if (item.mark == ware.mark){
            throw KilnError("duplicate mark");
        }
        if (item.kind == WareKind::Sagger){
            throw KilnError("sagger occupies the whole kiln");
        }
    }
    if (ware.kind == WareKind::Sagger and !stack_.empty()){
        throw KilnError("sagger needs an empty kiln");
    }
    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("not enough space");
    }
    stack_.push_back(ware);
    glazeSeen_.push_back(false);
}


void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open){
        throw KilnError("door is closed");
    }

    if (stack_.empty()){
        throw KilnError("kiln is empty");
    }

    const Ware& top = stack_.back();
    if (top.mark != mark){
        throw KilnError("not the top ware");
    }

    if (top.program == Program::Glaze and cone_ != Cone::Cold){
        throw KilnError("glaze can be taken only from a cold kiln");
    }
    stack_.pop_back();
    glazeSeen_.pop_back();
}


}  // namespace skopin
