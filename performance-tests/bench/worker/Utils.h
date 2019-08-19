#pragma once

#include "BenchC.h"

#include <iostream>

bool json_2_config(std::istream& is, Bench::WorkerConfig& config);
bool report_2_json(const Bench::WorkerReport& report, std::ostream& os);

