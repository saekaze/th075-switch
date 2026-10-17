// Switch port: the few SDL3 calls the upstream TH075 host uses, on SDL2 (the
// devkitPro portlib). Only the forms the host actually calls are provided.
#pragma once
#include <SDL.h>
#include <cstddef>
#include <cstdint>
using SDL_IOStream=SDL_RWops;
inline SDL_IOStream* SDL_IOFromFile(const char* path,const char* mode){return SDL_RWFromFile(path,mode);}
inline std::int64_t SDL_GetIOSize(SDL_IOStream* f){return SDL_RWsize(f);}
inline std::size_t SDL_ReadIO(SDL_IOStream* f,void* data,std::size_t size){return SDL_RWread(f,data,1,size);}
inline std::size_t SDL_WriteIO(SDL_IOStream* f,const void* data,std::size_t size){return SDL_RWwrite(f,data,1,size);}
inline bool SDL_CloseIO(SDL_IOStream* f){return SDL_RWclose(f)==0;}
