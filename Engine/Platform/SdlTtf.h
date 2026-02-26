#pragma once
#include "Platform/Sdl.h"

#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#elif __has_include(<SDL_ttf.h>)
#include <SDL_ttf.h>
#else
#error "SDL_ttf headers not found. Check vcpkg integration."
#endif