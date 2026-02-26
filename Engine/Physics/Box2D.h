#pragma once

#if __has_include(<box2d/box2d.h>)
#include <box2d/box2d.h>
#if __has_include(<box2d/math_functions.h>)
#include <box2d/math_functions.h>
#endif
#else
#error "Box2D headers not found. Check vcpkg + include paths."
#endif