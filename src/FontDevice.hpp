#pragma once
#include "GraphicsDevice.hpp"
#include "../game/TextSurface.hpp"
#include <memory>
namespace th075::sdl {
// Switch port of th075_web/cpp/sdl/FontDevice. Upstream ships the original
// GDI glyphs (MS Gothic, GGO_GRAY4, heights 12-32) as a pre-baked bank; here
// each glyph is rendered with FreeType from the player's msgothic.ttc (or the
// console's Japanese font) the first time it is drawn, then kept. A GG75 bank
// (original-glyphs.bin) in the data folder is used instead when present.
class FontDevice {
 GraphicsDevice& graphics;GlyphBank bank;struct Impl;std::unique_ptr<Impl> impl;
public:
 std::string error;
 explicit FontDevice(GraphicsDevice& g);~FontDevice();
 bool initialize();u32 create(const std::string&,const TextRasterOptions&);
 bool write(u32 texture,const std::string&,const TextRasterOptions&);
 bool raster(const std::string&,const TextRasterOptions&,std::vector<u8>& pixels);   // CP932 text -> BGRA pixels
};
}
