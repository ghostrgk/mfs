#pragma once

#include <stdio.h>

#define FSC_LOG(subsystem, msg)                  \
  do {                                           \
    fprintf(stderr, "%s: %s\n", subsystem, msg); \
  } while (0)
