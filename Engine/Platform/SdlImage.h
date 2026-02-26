#pragma once
#include "Platform/Sdl.h"

#if __has_include(<SDL2/SDL_image.h>)
#include <SDL2/SDL_image.h>
#elif __has_include(<SDL_image.h>)
#include <SDL_image.h>
#else
#error "SDL_image headers not found. Check vcpkg integration."
#endif