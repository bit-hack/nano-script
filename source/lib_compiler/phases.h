#pragma once
#include "ccml.h"

namespace ccml {

void run_sema(ccml_t &ccml);

void run_optimize(ccml_t &ccml);

void run_pre_codegen(ccml_t &ccml);

} // namespace ccml
