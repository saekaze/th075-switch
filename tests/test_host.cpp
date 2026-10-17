// Host-side tests for the Switch port layer (Linux; no game data needed).
//   archives  - th075.dat / th075b.dat indices (XOR-coded directory) read in
//               place; patched resources fall back to the 1.00 archive
//   music     - th075bgm.dat WAV entries: data chunk, format and the cue/adtl
//               loop, streamed as PCM
//   glyphs    - CP932 text rendered with on-demand FreeType glyphs (needs a
//               Japanese font: IPA Gothic stands in for MS Gothic)
//   renderer  - the SDL2/GLES3 port of the shared renderer clears, reads back
//               and presents (needs a display, e.g. Xvfb)
#include "../src/FileResources.hpp"
#include "../src/BgmStream.hpp"
#include "../src/FontDevice.hpp"
#include "../src/Platform.hpp"
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace th075;
static int failures=0;
#define CHECK(cond) do{if(!(cond)){std::printf("  FAIL %s:%d: %s\n",__FILE__,__LINE__,#cond);++failures;}}while(0)

namespace {
struct File {std::string name;std::vector<u8> bytes;};
// An archive as the 1.11 game writes it: u16 count, count 108-byte records
// (name[100], size, offset) XOR-coded with a running key, then the files.
std::vector<u8> archive(const std::vector<File>& files){
    const u32 table=2+u32(files.size())*108;std::vector<u8> out(table);out[0]=u8(files.size());out[1]=u8(files.size()>>8);u32 at=table;
    for(size_t i=0;i<files.size();++i){u8* r=out.data()+2+i*108;std::memcpy(r,files[i].name.c_str(),files[i].name.size());const u32 size=u32(files[i].bytes.size());std::memcpy(r+100,&size,4);std::memcpy(r+104,&at,4);at+=size;}
    u8 key=0x64,step=0x64;for(u32 i=2;i<table;++i){out[i]^=key;key=u8(key+step);step=u8(step+0x4d);}
    for(const auto& f:files)out.insert(out.end(),f.bytes.begin(),f.bytes.end());return out;
}
std::vector<u8> bytes(const char* s){return std::vector<u8>(s,s+std::strlen(s));}
void put32(std::vector<u8>& b,u32 v){for(int i=0;i<4;++i)b.push_back(u8(v>>(8*i)));}
void tag(std::vector<u8>& b,const char* t){b.insert(b.end(),t,t+4);}
// A 16-bit stereo WAV of `frames` frames (sample n = n, -n), with optional
// cue (loop start) and LIST/adtl/ltxt (loop length) chunks after the data.
std::vector<u8> wave(u32 frames,u32 loop_start,u32 loop_length){
    std::vector<u8> b;tag(b,"RIFF");put32(b,0);tag(b,"WAVE");
    tag(b,"fmt ");put32(b,16);b.push_back(1);b.push_back(0);b.push_back(2);b.push_back(0);put32(b,44100);put32(b,44100*4);b.push_back(4);b.push_back(0);b.push_back(16);b.push_back(0);
    tag(b,"data");put32(b,frames*4);for(u32 n=0;n<frames;++n){const i16 l=i16(n),r=i16(-i32(n));b.push_back(u8(l));b.push_back(u8(u16(l)>>8));b.push_back(u8(r));b.push_back(u8(u16(r)>>8));}
    if(loop_start){tag(b,"cue ");put32(b,28);put32(b,1);put32(b,1);put32(b,loop_start);tag(b,"data");put32(b,0);put32(b,0);put32(b,loop_start);
        tag(b,"LIST");put32(b,4+8+20);tag(b,"adtl");tag(b,"ltxt");put32(b,20);put32(b,1);put32(b,loop_length);tag(b,"rgn ");put32(b,0);put32(b,0);}
    const u32 riff=u32(b.size()-8);std::memcpy(b.data()+4,&riff,4);return b;
}
}

static void test_archives(const std::string& dir){
    std::puts("archives (th075.dat + th075b.dat)");
    host::paths().data=host::paths().save=dir;
    CHECK(host::write_file(dir+"/th075.dat",archive({{"data/system/a.dat",bytes("BASE-A")},{"data\\system\\b.dat",bytes("BASE-B")},{"wave/empty.dat",{}}})));
    CHECK(host::write_file(dir+"/th075b.dat",archive({{"data/system/b.dat",bytes("PATCH-B")}})));
    sdl::FileResources files;CHECK(files.open());std::vector<u8> out;
    CHECK(files.read(ResourceEdition::Patched,"data/system/b.dat",out)&&out==bytes("PATCH-B"));
    CHECK(files.read(ResourceEdition::Patched,"data/system/a.dat",out)&&out==bytes("BASE-A"));   // not patched: the 1.00 file
    CHECK(files.read(ResourceEdition::Base,"data/system/b.dat",out)&&out==bytes("BASE-B"));
    CHECK(files.read(ResourceEdition::Base,"data\\system\\a.dat",out)&&out==bytes("BASE-A"));
    CHECK(files.read(ResourceEdition::Base,"wave/empty.dat",out)&&out.empty());
    CHECK(!files.read(ResourceEdition::Base,"data/missing.dat",out));
    CHECK(!files.read(ResourceEdition::Base,"../th075.dat",out));
    // Saves: upstream's /save paths are the SD folder.
    CHECK(sdl::FileResources::host_path("/save/replay/replay000.rep")==dir+"/replay/replay000.rep");
    CHECK(sdl::FileResources::write_file("/save/config.dat","CFG",3)&&host::exists(dir+"/config.dat"));
    std::printf("  index, patch fallback, path checks and save folder: %s\n",failures?"FAILED":"ok");
}

static void test_music(const std::string& dir){
    std::puts("music (th075bgm.dat WAV loops)");
    const auto looped=wave(1000,200,500),plain=wave(300,0,0),ending=wave(400,100,200);
    CHECK(host::write_file(dir+"/th075bgm.dat",archive({{"wave/bgm/01a.wav",looped},{"wave/bgm/02b.wav",plain},{"wave/bgm/sys99_ed.wav",ending}})));
    sdl::FileResources::Archive bgm;std::string error;CHECK(sdl::FileResources::open_archive(bgm,dir+"/th075bgm.dat",error));
    const auto track=[&](i16 cue,host::MusicTrack& t){const auto* e=bgm.index.find(music_resource_path(cue));return e&&host::locate_music(bgm.file,e->offset,e->size,cue,t,error);};
    host::MusicTrack t;
    CHECK(track(1,t));CHECK(t.channels==2&&t.rate==44100&&t.start==200&&t.end==700&&t.loop);
    CHECK(track(12,t));CHECK(t.start==0&&t.end==300&&t.loop);   // no cue: the whole track
    CHECK(track(41,t));CHECK(t.start==100&&t.end==300&&!t.loop);  // the ending theme plays once
    // Stream the looped track through the data source: frame n is (n, -n).
    CHECK(track(1,t));host::BgmFile file{bgm.file};host::PcmStream s;s.source=&file;s.offset=t.offset;s.frames=t.end;
    auto config=ma_data_source_config_init();config.vtable=&host::pcm_vtable;CHECK(ma_data_source_init(&config,&s.base)==MA_SUCCESS);
    float pcm[8];ma_uint64 read=0;CHECK(ma_data_source_seek_to_pcm_frame(&s,698)==MA_SUCCESS);ma_data_source_read_pcm_frames(&s,pcm,4,&read);
    std::printf("  frames 698..: %llu read, %.0f %.0f %.0f %.0f\n",(unsigned long long)read,pcm[0]*32768,pcm[1]*32768,pcm[2]*32768,pcm[3]*32768);
    CHECK(read==2&&std::lround(pcm[0]*32768)==698&&std::lround(pcm[1]*32768)==-698&&std::lround(pcm[2]*32768)==699);
    ma_data_source_uninit(&s.base);
}

static void test_glyphs(const std::string& dir){
    std::puts("glyphs (CP932 text, FreeType on demand)");
    const char* fonts[]{"/usr/share/fonts/opentype/ipafont-gothic/ipag.ttf","/usr/share/fonts/truetype/fonts-japanese-gothic.ttf"};std::vector<u8> font;
    for(const char* f:fonts)if(host::read_file(f,font,64*1024*1024))break;
    if(font.empty()){std::puts("  skipped: no Japanese font");return;}
    CHECK(host::write_file(dir+"/msgothic.ttc",font));
    sdl::GraphicsDevice graphics;sdl::FontDevice fonts_(graphics);CHECK(fonts_.initialize());
    // "東方 A" in CP932, then a colour command and a second line.
    const std::string text="\x93\x8c\x95\xfb A\\cff0000\xb1\\n2";
    TextRasterOptions o;o.width=256;o.height=64;o.wrapWidth=240;o.fontHeight=24;std::vector<u8> pixels;
    CHECK(fonts_.raster(text,o,pixels));CHECK(pixels.size()==256*64*4);
    u32 lit=0,first=0,second=0;i32 top=64,bottom=-1;
    for(u32 y=0;y<64;++y)for(u32 x=0;x<256;++x){const u8* p=&pixels[(y*256+x)*4];if(p[3]){++lit;top=std::min<i32>(top,i32(y));bottom=std::max<i32>(bottom,i32(y));if(x<25)++first;else if(x>=25&&x<49)++second;}}
    std::printf("  %u lit pixels (東 %u, 方 %u), rows %d-%d\n",lit,first,second,top,bottom);
    CHECK(first>20&&second>20);CHECK(top>=0&&top<8&&bottom>24);   // two lines: the \n moved down a row
    // Unmapped or blank codes draw nothing but do not fail.
    CHECK(fonts_.raster("\x81\x40\x85\x40",o,pixels));
}

static bool test_renderer(){
    std::puts("renderer (SDL2 + GLES3)");
    if(!std::getenv("DISPLAY")&&!std::getenv("WAYLAND_DISPLAY")){std::puts("  skipped: no display");return false;}
    sdl::GraphicsDevice graphics;const bool ok=graphics.initialize(1);CHECK(ok);if(!ok){std::printf("  %s\n",graphics.error.c_str());return true;}
    graphics.clear(0xff0000ff);const u8* p=graphics.capture_pixels();CHECK(p!=nullptr);
    if(p)std::printf("  clear %02x%02x%02x\n",p[(240*640+320)*4+2],p[(240*640+320)*4+1],p[(240*640+320)*4]);
    if(p)CHECK(p[(240*640+320)*4]==0xff&&p[(240*640+320)*4+2]==0);
    graphics.present();
    const auto& pic=graphics.renderer.picture;std::printf("  picture %dx%d at %d,%d in %dx%d\n",pic.width,pic.height,pic.x,pic.y,pic.drawable_width,pic.drawable_height);
    CHECK(pic.width*3==pic.height*4);return true;
}

int main(){
    const std::string dir=std::string(std::getenv("TMPDIR")?std::getenv("TMPDIR"):"/tmp")+"/th075-switch-test";
    host::make_directory(dir);host::make_directory(dir+"/replay");
    test_archives(dir);test_music(dir);test_glyphs(dir);
    if(SDL_Init(SDL_INIT_VIDEO)==0){test_renderer();SDL_Quit();}else std::printf("renderer skipped: %s\n",SDL_GetError());
    std::printf(failures?"FAILED (%d)\n":"all host tests passed\n",failures);
    return failures?1:0;
}
