#pragma once

#include "microprofile.h"
#include "math.h"
#define MICROPROFILE_SCOPEIC(group, name) MICROPROFILE_SCOPEI(group,name,randcolor())
