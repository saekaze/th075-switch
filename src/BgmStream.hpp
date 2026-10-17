// Switch host: a th075bgm.dat track (the PCM of one WAV entry) as a miniaudio
// data source. Same as th15-switch's thbgm.dat stream.
#pragma once
#include "MiniaudioConfig.hpp"
#include "Platform.hpp"
#include <cstdio>
#include <string>
#include <vector>
namespace th075::host {
// One open th075bgm.dat shared by every track; `position` is where the FILE
// currently is, so a track only seeks when another one moved it.
struct BgmFile {FILE* file=nullptr;u64 position=~0ull;};
struct PcmStream {
    ma_data_source_base base{};BgmFile* source=nullptr;u64 offset=0,frames=0,cursor=0;u32 channels=2,rate=44100;
    std::vector<i16> scratch;
};
extern const ma_data_source_vtable pcm_vtable;
// Where one track's PCM sits in th075bgm.dat and how it loops: the WAV entry's
// data chunk and format, and its loop from the cue (loop start frame) and
// adtl/ltxt (loop length) chunks - read exactly like upstream's
// prepare-music.py (including its fixed loop for cue 59). The track ends at
// the loop end; only the ending theme (cue 41) does not loop.
struct MusicTrack {u64 offset=0;u32 channels=0,rate=0,start=0,end=0;bool loop=true;};
bool locate_music(std::FILE* archive,u64 entry_offset,u32 entry_size,i16 cue,MusicTrack& out,std::string& error);
}
