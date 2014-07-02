#include "registry.h"
static const Pb::Register _reg("python/defines.py", "################################################################################\n#\n# MantaFlow fluid solver framework\n# Copyright 2011 Tobias Pfaff, Nils Thuerey \n#\n# This program is free software, distributed under the terms of the\n# GNU General Public License (GPL) \n# http://www.gnu.org/licenses\n#\n# Defines some constants for use in python subprograms\n#\n#################################################################################\n\n# mantaflow conventions\nReal = float\n\n# grid flags\nFlagFluid = 1\nFlagObstacle = 2\nFlagEmpty = 4\nFlagStick = 128\nFlagReserved = 256\n\n# integration mode\nIntEuler = 0\nIntRK2 = 1\nIntRK4 = 2\n\n\n\n");
