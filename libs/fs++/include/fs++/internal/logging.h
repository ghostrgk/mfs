#pragma once

#include <iostream>

#define FSPP_LOG(subsystem, msg) std::cerr << (subsystem) << ": " << (msg) << std::endl
