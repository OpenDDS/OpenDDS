#pragma once

#include "BenchC.h"

#include <iostream>

void set_global_properties(Builder::PropertySeq& properties);
const Builder::PropertySeq& get_global_properties();

bool json_2_config(std::istream& is, Bench::WorkerConfig& config);
bool report_2_json(const Bench::WorkerReport& report, std::ostream& os);

