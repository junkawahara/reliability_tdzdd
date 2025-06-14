#pragma once

#ifndef B_64
#if __SIZEOF_POINTER__ == 8
#define B_64
#endif
#endif
#include <stdexcept>
#include <BDD.h>

#include "tdzdd/DdEval.hpp"

namespace tdzdd {

/**
 * Exporter to BDD.
 * shift_vars is a vector that maps TdZdd node levels to corresponding BDD variable levels.
 * When the BDD variables are not enough, they are created automatically by BDD_NewVar().
 */
struct ToShiftedBDD: public tdzdd::DdEval<ToShiftedBDD,BDD> {
    // shift_vars: maps input BDD variable levels to output BDD variable levels
    std::vector<int> shift_vars;

public:
    ToShiftedBDD(const std::vector<int>& shift_vars)
            : shift_vars(shift_vars) {
        if (shift_vars.empty()) {
            throw std::runtime_error("ERROR: shift_vars must not be empty.");
        }
        // check shift_vars are sorted in ascending order
        for (size_t i = 1; i < shift_vars.size(); ++i) {
            if (shift_vars[i] <= shift_vars[i - 1]) {
                throw std::runtime_error("ERROR: shift_vars must be sorted in ascending order.");
            }
        }
    }

    void initialize(int topLevel) const {
        while (BDD_VarUsed() < topLevel) {
            BDD_NewVar();
        }
    }

    void evalTerminal(BDD& f, int value) const {
        f = BDD(value);
    }

    void evalNode(BDD& f, int level, tdzdd::DdValues<BDD,2> const& values) const {        
        BDD f0 = values.get(0);
        BDD f1 = values.get(1);
        int var = BDD_VarOfLev(shift_vars[level]);
        f = (f0 & ~BDDvar(var)) | (f1 & BDDvar(var));
    }
};

} // namespace tdzdd
