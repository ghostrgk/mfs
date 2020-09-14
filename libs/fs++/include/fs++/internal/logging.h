#pragma once

#include <iostream>

#define FSC_LOG(subsystem, msg) std::cerr << (subsystem) << ": " << (msg) << std::endl
