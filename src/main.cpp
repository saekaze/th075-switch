// Touhou 7.5: Immaterial and Missing Power for Nintendo Switch: native entry
// point.
//
// This replaces th075_web/cpp/sdl/BrowserRuntime.cpp. The Application it
// drives is upstream's (title, story, arcade, versus, training, replays,
// music room, endings); the browser parts become: the original archives on
// the SD card, a vsync-paced 60 Hz loop instead of requestAnimationFrame, and
// two Switch controllers as the two players, fed through the same held bits
// (left, right, up, down, A, B, C, D) the web launcher's joystick path uses.
#include "Application.hpp"
#include "Platform.hpp"
#include "SdlCompat.hpp"
#include "../portable/sdl/FrameCadence.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#ifdef __SWITCH__
#include <switch.h>
#endif

namespace th075::sdl {
namespace {
std::unique_ptr<Application> app;

// Controls, the same for both players (laid out like the other Saekaze ports,
// where B is the main button):
//   B = A (weak attack, menu confirm)   A = B (strong attack, menu cancel)
//   Y = C (spell card)                  X = D (dash / graze)
//   ZL/L = C, ZR/R = D as well          + = that player's pause / start
//   Stick or d-pad: move               − = snapshot
// Player 1 is the Joy-Cons (handheld, in a grip or both detached), player 2
// any other controller such as a Pro Controller. With no Joy-Cons connected,
// the first other controller is player 1 and the next one player 2.
// Controller 1 / 2 play the active / other side of the battle.
struct Pad {bool connected=false;u8 held=0;bool start=false,minus=false;};
Pad pads[2];
#ifdef __SWITCH__
constexpr HidNpadIdType pad_ids[9]{HidNpadIdType_Handheld,HidNpadIdType_No1,HidNpadIdType_No2,HidNpadIdType_No3,HidNpadIdType_No4,HidNpadIdType_No5,HidNpadIdType_No6,HidNpadIdType_No7,HidNpadIdType_No8};
PadState pad_states[9];
void initialize_pads(){
    padConfigureInput(8,HidNpadStyleSet_NpadStandard);
    for(u32 n=0;n<9;++n)padInitialize(&pad_states[n],pad_ids[n]);
}
Pad read(PadState& ps){
    Pad p;p.connected=true;const u64 held=padGetButtons(&ps);const auto stick=padGetStickPos(&ps,0);
    // Half-way stick or a d-pad press, like the web path's stick test.
    const float x=float(stick.x)/32767.f,y=-float(stick.y)/32767.f;const bool tilted=x*x+y*y>=.25f;
    if(held&HidNpadButton_Left||(tilted&&x<-.38f))p.held|=1;if(held&HidNpadButton_Right||(tilted&&x>.38f))p.held|=2;
    if(held&HidNpadButton_Up||(tilted&&y<-.38f))p.held|=4;if(held&HidNpadButton_Down||(tilted&&y>.38f))p.held|=8;
    if(held&HidNpadButton_B)p.held|=16;if(held&HidNpadButton_A)p.held|=32;
    if(held&(HidNpadButton_Y|HidNpadButton_L|HidNpadButton_ZL))p.held|=64;if(held&(HidNpadButton_X|HidNpadButton_R|HidNpadButton_ZR))p.held|=128;
    p.start=held&HidNpadButton_Plus;p.minus=held&HidNpadButton_Minus;return p;
}
void update_pads(){
    pads[0]=pads[1]={};Pad joycons,others[2];u32 other_count=0;
    for(auto& ps:pad_states){padUpdate(&ps);if(!padIsConnected(&ps))continue;const u32 style=padGetStyleSet(&ps);const Pad p=read(ps);
        if(style&(HidNpadStyleTag_NpadHandheld|HidNpadStyleTag_NpadJoyDual|HidNpadStyleTag_NpadJoyLeft|HidNpadStyleTag_NpadJoyRight)){
            joycons.connected=true;joycons.held|=p.held;joycons.start|=p.start;joycons.minus|=p.minus;}
        else if(other_count<2)others[other_count++]=p;}
    if(joycons.connected){pads[0]=joycons;pads[1]=others[0];}else{pads[0]=others[0];pads[1]=others[1];}
}
#else
// Desktop test build: player 1 on the keyboard (arrows, Z X C V, Enter =
// pause, F12 = snapshot).
void initialize_pads(){}
void update_pads(){
    const auto* k=SDL_GetKeyboardState(nullptr);auto& p=pads[0];p={};p.connected=true;
    const SDL_Scancode keys[8]{SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_UP,SDL_SCANCODE_DOWN,SDL_SCANCODE_Z,SDL_SCANCODE_X,SDL_SCANCODE_C,SDL_SCANCODE_V};
    for(u32 b=0;b<8;++b)if(k[keys[b]])p.held|=u8(1u<<b);p.start=k[SDL_SCANCODE_RETURN];p.minus=k[SDL_SCANCODE_F12];
}
#endif

std::array<ControlCounts,2> control_counts;std::array<i32,2> cancel_counts{};
bool snapshot_held=false,snapshot_requested=false;

BattleSystemInput sample(){
    BattleSystemInput input;update_pads();
    const u8 active=app->selection.activePlayer<2?app->selection.activePlayer:0;
    u8 held[2]{};bool start[2]{};
    for(u32 n=0;n<2;++n){if(!pads[n].connected)continue;const u32 side=n==0?active:1u-active;held[side]|=pads[n].held;start[side]|=pads[n].start;}
    for(u32 side=0;side<2;++side){
        control_counts[side].from_held(held[side]);input.players[side]=control_counts[side];
        cancel_counts[side]=start[side]?increment_wrapped(cancel_counts[side],1):0;input.cancel[side]=cancel_counts[side];
    }
    const bool minus=pads[0].minus||pads[1].minus;if(minus&&!snapshot_held)snapshot_requested=true;snapshot_held=minus;
    return input;
}
void clear_inputs(){for(auto& c:control_counts)c.clear();cancel_counts.fill(0);}

// Switch port: '−' saves the current frame as a BMP into snapshot/ next to
// the saves, like the PC game's snapshot folder.
bool snapshot(){
    const u8* pixels=app->graphics.capture_pixels();if(!pixels)return false;
    const auto dir=host::paths().save_file("snapshot");host::make_directory(dir);char file[64];std::string path;
    for(u32 n=0;n<1000;++n){std::snprintf(file,sizeof(file),"/th075_%03u.bmp",n);path=dir+file;if(!host::exists(path))break;}
    const u32 w=640,h=480,row=w*3,size=54+row*h;std::vector<u8> bmp(size,0);
    auto put=[&](u32 at,u32 v,u32 n){for(u32 i=0;i<n;++i)bmp[at+i]=u8(v>>(8*i));};
    bmp[0]='B';bmp[1]='M';put(2,size,4);put(10,54,4);put(14,40,4);put(18,w,4);put(22,h,4);put(26,1,2);put(28,24,2);put(34,size-54,4);
    for(u32 y=0;y<h;++y){const u8* src=pixels+(h-1-y)*w*4;u8* dst=bmp.data()+54+y*row;for(u32 x=0;x<w;++x){dst[x*3]=src[x*4];dst[x*3+1]=src[x*4+1];dst[x*3+2]=src[x*4+2];}}
    const bool ok=host::write_file(path,bmp);host::log("snapshot %s: %s",path.c_str(),ok?"ok":"failed");return ok;
}

void fatal(const std::string& message){
    host::log("fatal: %s",message.c_str());
    // Tear down the game (GL context, window, audio) so the console can take
    // the framebuffer, then wait for +.
    app.reset();SDL_Quit();
#ifdef __SWITCH__
    consoleInit(nullptr);
    std::printf("\n  Touhou 7.5: Immaterial and Missing Power - Switch port\n\n  Error: %s\n\n",message.c_str());
    std::printf("  Data folder: %s\n  Needed: th075.dat and th075b.dat (ver1.11), msgothic.ttc.\n  Optional: th075bgm.dat (music)\n\n  Press + to exit.\n",host::paths().data.c_str());
    consoleUpdate(nullptr);
    PadState pad;padInitializeDefault(&pad);
    while(appletMainLoop()){padUpdate(&pad);if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;consoleUpdate(nullptr);}
    consoleExit(nullptr);
#else
    std::fprintf(stderr,"%s\n",message.c_str());
#endif
}
}
}

int main(int argc,char** argv){
    using namespace th075::sdl;namespace host=th075::host;using th075::u32;
    host::locate_data(argc,argv);
#ifdef __SWITCH__
    appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleep);
#endif
    initialize_pads();
    host::log("th075-switch: data %s",host::paths().data.c_str());
    if(!host::exists(host::paths().data_file("th075.dat"))){fatal("th075.dat not found");return 1;}
    app=std::make_unique<Application>();
    // Render scale 2: the 640x480 game drawn at 1280x960, then fitted to the screen.
    const double begin=host::seconds();
    if(!app->initialize(2)){fatal(app->error.empty()?"start-up failed":app->error);return 1;}
    host::log("game initialised in %.1fs",host::seconds()-begin);
    clear_inputs();
    // Vsync paces the loop at the panel's 60 Hz: one game tick per swap.
    // Measured time only adds catch-up ticks after a real stall (bounded to
    // 4 like upstream FrameCadence), so vsync jitter never drops a frame.
    constexpr double interval=touhou::sdl::FrameCadence::interval;
    double debt=0,previous=host::seconds();bool running=true,ok=true;
    while(running&&ok
#ifdef __SWITCH__
          &&appletMainLoop()
#endif
    ){
        SDL_Event e;while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT)running=false;
            else if(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_FOCUS_LOST)clear_inputs();
        }
        const double now=host::seconds();debt+=std::clamp(now-previous,0.,.1);previous=now;
        u32 ticks=u32(std::floor((debt+interval*.5)/interval));ticks=std::min(4u,std::max(ticks,1u));
        debt=std::clamp(debt-ticks*interval,-interval*.5,.1);
        app->graphics.renderer.defer=true;
        for(u32 n=0;n<ticks&&ok;++n){ok=app->tick(sample(),u32(SDL_GetTicks()));if(app->exiting)break;}
        if(ok&&snapshot_requested){snapshot_requested=false;snapshot();}
        app->graphics.renderer.commit();app->graphics.renderer.defer=false;app->audio.pump();
        if(app->exiting){running=false;ok=true;} // title menu Exit
    }
    if(!ok&&!app->exiting){fatal(app->error.empty()?"game stopped":app->error);return 1;}
    app->save_scores();
    app.reset();
    SDL_Quit();
    return 0;
}
