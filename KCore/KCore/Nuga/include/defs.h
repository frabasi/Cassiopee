/*
 
 
 
              NUGA 
 
 
 
 */

#ifndef __NUGA_DEFS_H__
#define __NUGA_DEFS_H__

#define ZERO_M       1.e-15
#define EPSILON      1.e-12
#define IDX_NONE     2147483647

#define Vector_t     std::vector
#define SQRT         std::sqrt

// types
#ifndef NUGALIB

#include "Def/DefTypes.h"
#include "Def/DefCplusPlusConst.h"

namespace NUGA
{
static const E_Float FLOAT_MAX = K_CONST::E_MAX_FLOAT;
static const E_Float SQRT3 = K_CONST::E_SQRT3;

static const E_Float PI   = K_CONST::E_PI;
static const E_Float PI_2 = K_CONST::E_PI_2;
static const E_Float PI_4 = K_CONST::E_PI_4;
}

#else

using E_Float = double;
using E_Int   = int;
using E_Bool  = int;

#include <limits>
namespace NUGA
{
  static const E_Float FLOAT_MAX = std::numeric_limits<E_Float>::max();
  static const E_Float SQRT3     = 1.73205080756887729353;

  static const E_Float PI        = 3.14159265358979323846;
  static const E_Float PI_2      = 1.57079632679489661923;
  static const E_Float PI_4      = 0.78539816339744830962;
}

#endif

#endif
