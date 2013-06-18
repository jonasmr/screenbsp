#pragma once

#include "microprofile.h"
#include "math.h"
#define ZMICROPROFILE_SCOPEIC(group, name) ZMICROPROFILE_SCOPEI(group,name,randcolor())
