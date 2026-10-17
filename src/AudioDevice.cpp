// Switch port of th075_web/cpp/sdl/AudioDevice.cpp. Same miniaudio mixer,
// sound banks, volume curve, fades and loop points as upstream; two host
// changes: SDL2 queued audio instead of an SDL3 audio stream, and the music is
// streamed straight out of the original th075bgm.dat (16-bit PCM WAV entries,
// loop points from each WAV's cue/adtl chunks exactly as upstream's
// prepare-music.py reads them) instead of pre-converted OGG files.
#define MINIAUDIO_IMPLEMENTATION
#include "MiniaudioConfig.hpp"
#include "AudioDevice.hpp"
#include "BgmStream.hpp"
#include "FileResources.hpp"
#include "Platform.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
namespace th075::sdl {
namespace {float gain(i32 level,i32 volume){level+=(volume-100)*50;if(volume==0||level< -10000)level=-10000;if(level>0)level=0;return std::pow(10.f,float(level)/2000.f);}}
struct AudioDevice::Impl {
 struct Voice {std::vector<u8> bytes;ma_decoder decoder{};host::PcmStream pcm;ma_sound sound{};bool decoded=false,streamed=false,attached=false;~Voice(){if(attached)ma_sound_uninit(&sound);if(decoded)ma_decoder_uninit(&decoder);if(streamed)ma_data_source_uninit(&pcm.base);}};
 ma_engine engine{};SDL_AudioDeviceID device=0;bool engineReady=false,ready=false,refill=true,suspended=false;
 FileResources::Archive bgm;host::BgmFile bgmFile;
 std::array<std::vector<std::unique_ptr<Voice>>,3> banks;std::unique_ptr<Voice> current,pending;
 i16 currentCue=-1,pendingCue=-1;i32 level=0,target=-10000,step=-160;u8 stopWhenDone=1,startWhenDone=0;bool fading=false;u32 lastFade=0;
 ~Impl(){current.reset();pending.reset();for(auto& bank:banks)bank.clear();if(device)SDL_CloseAudioDevice(device);if(engineReady)ma_engine_uninit(&engine);}
 bool attach(Voice& v){ma_data_source* source=v.streamed?static_cast<ma_data_source*>(&v.pcm):static_cast<ma_data_source*>(&v.decoder);if(ma_sound_init_from_data_source(&engine,source,MA_SOUND_FLAG_NO_SPATIALIZATION,nullptr,&v.sound)!=MA_SUCCESS)return false;v.attached=true;return true;}
 bool bank(u8 index,const SoundPack& pack){std::vector<std::unique_ptr<Voice>> next(pack.samples.size());for(u32 i=0;i<pack.samples.size();i++){if(!pack.samples[i].present)continue;auto voice=std::make_unique<Voice>();voice->bytes=pack.samples[i].wave_file();auto config=ma_decoder_config_init(ma_format_f32,2,44100);if(ma_decoder_init_memory(voice->bytes.data(),voice->bytes.size(),&config,&voice->decoder)!=MA_SUCCESS)return false;voice->decoded=true;if(!attach(*voice))return false;next[i]=std::move(voice);}banks[index]=std::move(next);return true;}
 void play(u8 bank,i16 id,i32 volume){if(id<0||u32(id)>=banks[bank].size()||!banks[bank][id])return;auto& v=*banks[bank][id];ma_sound_stop(&v.sound);ma_sound_seek_to_pcm_frame(&v.sound,0);ma_sound_set_volume(&v.sound,gain(0,volume));ma_sound_start(&v.sound);}
 bool playing()const{return current&&ma_sound_is_playing(&current->sound);}
 std::unique_ptr<Voice> music(i16 cue,std::string& error){
  const auto path=music_resource_path(cue);const auto* e=bgm.index.find(path);if(!e){error="Missing music "+path+" in th075bgm.dat";return nullptr;}
  host::MusicTrack track;bgmFile.position=~0ull;if(!host::locate_music(bgm.file,e->offset,e->size,cue,track,error)){error=path+": "+error;return nullptr;}
  auto next=std::make_unique<Voice>();auto& s=next->pcm;s.source=&bgmFile;s.offset=track.offset;s.frames=track.end;s.channels=track.channels;s.rate=track.rate;
  auto config=ma_data_source_config_init();config.vtable=&host::pcm_vtable;if(ma_data_source_init(&config,&s.base)!=MA_SUCCESS){error="Unable to open music stream";return nullptr;}next->streamed=true;
  if(track.loop)ma_data_source_set_loop_point_in_pcm_frames(&s,track.start,track.end);
  if(!attach(*next)){error="Unable to attach music";return nullptr;}ma_sound_set_looping(&next->sound,track.loop?MA_TRUE:MA_FALSE);return next;
 }
};
AudioDevice::AudioDevice():impl(std::make_unique<Impl>()){}AudioDevice::~AudioDevice()=default;void AudioDevice::close(){impl=std::make_unique<Impl>();}
bool AudioDevice::initialize(ResourceStore& resources){
 auto& a=*impl;if(a.ready)return true;auto cfg=ma_engine_config_init();cfg.noDevice=MA_TRUE;cfg.channels=2;cfg.sampleRate=44100;cfg.defaultVolumeSmoothTimeInPCMFrames=0;
 if(ma_engine_init(&cfg,&a.engine)!=MA_SUCCESS){error="Unable to initialize audio mixer";return false;}a.engineReady=true;
 if(SDL_InitSubSystem(SDL_INIT_AUDIO)==0){SDL_AudioSpec want{},have{};want.freq=44100;want.format=AUDIO_F32SYS;want.channels=2;want.samples=2048;a.device=SDL_OpenAudioDevice(nullptr,0,&want,&have,0);}
 if(!a.device){error=SDL_GetError();return false;}
 std::vector<u8> data;SoundPack system;if(!resources.read(ResourceEdition::Base,"wave/se.dat",data)||!system.load(data.data(),data.size())||!a.bank(0,system)){error="Invalid system sound pack";return false;}
 // th075bgm.dat is optional: without it the game runs without music.
 std::string missing;if(FileResources::open_archive(a.bgm,host::paths().data_file("th075bgm.dat"),missing))a.bgmFile.file=a.bgm.file;else{host::log("%s: no music",missing.c_str());a.bgm=FileResources::Archive{};}
 SDL_PauseAudioDevice(a.device,0);a.ready=true;return true;
}
bool AudioDevice::character_bank(u8 side,const SoundPack& pack){if(side>1||!impl->ready)return false;if(!impl->bank(side+1,pack)){error="Unable to decode character sounds";return false;}return true;}
void AudioDevice::release_character(u8 side){if(side<2)impl->banks[side+1].clear();}
void AudioDevice::sound(i16 id){impl->play(0,id,soundVolume);}void AudioDevice::character_sound(u8 side,i16 id){if(side<2)impl->play(side+1,id,soundVolume);}
bool AudioDevice::select_music(i16 cue){auto& a=*impl;if(!a.ready||cue<0)return false;if(!a.bgm.file)return true;auto next=a.music(cue,error);if(!next)return false;a.pending=std::move(next);a.pendingCue=cue;return true;}
void AudioDevice::stop_music(){auto& a=*impl;if(a.current)ma_sound_stop(&a.current->sound);a.fading=false;}
void AudioDevice::start_music(){auto& a=*impl;if(a.pending){if(a.current)ma_sound_stop(&a.current->sound);a.current=std::move(a.pending);a.currentCue=a.pendingCue;a.pendingCue=-1;}if(!a.current)return;a.level=0;a.fading=false;ma_sound_seek_to_pcm_frame(&a.current->sound,0);ma_sound_set_volume(&a.current->sound,gain(a.level,musicEnabled?musicVolume:0));ma_sound_start(&a.current->sound);}
void AudioDevice::music_level(i32 level){impl->level=level;if(impl->current)ma_sound_set_volume(&impl->current->sound,gain(level,musicEnabled?musicVolume:0));}
void AudioDevice::fade_music(i32 target,i32 step,u8 stopWhenDone,u8 startWhenDone){auto& a=*impl;if(!a.playing()){a.fading=false;if(startWhenDone)start_music();return;}a.target=target;a.step=step;a.stopWhenDone=stopWhenDone;a.startWhenDone=startWhenDone;a.fading=true;a.lastFade=u32(SDL_GetTicks());}
void AudioDevice::update(u32 now){auto& a=*impl;if(a.fading&&a.playing()&&u32(now-a.lastFade)>=80){a.lastFade+=80;if(a.current)ma_sound_set_volume(&a.current->sound,gain(a.level,musicEnabled?musicVolume:0));a.level=increment_wrapped(a.level,a.step*4);if(a.step<0){if(a.level<a.target||a.level< -9999)a.fading=false;}else if(a.level<a.target||a.level>=0){if(a.level>=0)a.level=0;a.fading=false;}if(!a.fading&&a.stopWhenDone){stop_music();if(a.startWhenDone)start_music();}}if(a.current&&!a.fading)ma_sound_set_volume(&a.current->sound,gain(a.level,musicEnabled?musicVolume:0));}
bool AudioDevice::mix(float* pcm,u32 frames){ma_uint64 read=0;if(!impl->ready||!pcm||ma_engine_read_pcm_frames(&impl->engine,pcm,frames,&read)!=MA_SUCCESS||read!=frames)return false;for(u32 i=0;i<frames*2;i++)pcm[i]=std::clamp(pcm[i],-1.f,1.f);return true;}
void AudioDevice::pump(){auto& a=*impl;if(!a.ready||!a.device||a.suspended)return;i32 queued=i32(SDL_GetQueuedAudioSize(a.device)/8);if(!a.refill&&queued<4096)a.refill=true;if(!a.refill)return;if(queued>=6144){a.refill=false;return;}for(i32 i=0;i<6&&queued<6144;i++){float pcm[2048]{};if(!mix(pcm,1024))return;if(SDL_QueueAudio(a.device,pcm,sizeof(pcm))!=0)return;queued+=1024;}if(queued>=6144)a.refill=false;}
void AudioDevice::suspend(bool suspend){auto& a=*impl;a.suspended=suspend;if(a.device)SDL_PauseAudioDevice(a.device,suspend?1:0);}
}
