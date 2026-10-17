#pragma once
#include "GraphicsDevice.hpp"
#include "../game/SpriteRenderer.hpp"
namespace th075::sdl {
class TextureBank final:public SpriteResources {
 GraphicsDevice& device;std::vector<SpriteImage> images;
public:
 std::vector<u32> handles;
 explicit TextureBank(GraphicsDevice& graphics):device(graphics){}
 ~TextureBank(){clear();}
 bool load(const TexturePack& pack){std::vector<u32> next;if(!device.preload(pack,next))return false;clear();handles=std::move(next);images.reserve(handles.size());for(u32 i=0;i<handles.size();i++)images.push_back({handles[i],pack.images[i].width,pack.images[i].height});return true;}
 void clear(){device.unload(handles);handles.clear();images.clear();}
 SpriteImage image(i16 n)override{return n>=0&&u32(n)<images.size()?images[n]:SpriteImage{};}
 u32 size()const{return u32(images.size());}
};
}
