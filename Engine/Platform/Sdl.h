#pragma once

// Prevent SDL from redefining main() in consuming apps (we use normal int main()).
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

// SDL include path differs by platform/package layout.
// This wrapper makes it compile either way.
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL headers not found. Check vcpkg integration and include paths."
#endif