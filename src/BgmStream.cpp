// Streams signed 16-bit interleaved PCM out of th075bgm.dat at a byte offset
// (a WAV data chunk), converted to f32 on read. Looping is left to
// miniaudio's data-source loop points, exactly like the upstream decoder path.
#include "BgmStream.hpp"
#include <algorithm>
#include <cstring>
namespace th075::host {
namespace {
ma_result pcm_read(ma_data_source* source,void* out,ma_uint64 count,ma_uint64* read){
    auto& s=*static_cast<PcmStream*>(source);if(read)*read=0;
    const u64 n=std::min<u64>(count,s.frames-std::min(s.cursor,s.frames));if(!n)return MA_AT_END;
    auto& f=*s.source;const u64 at=s.offset+s.cursor*s.channels*2;
    if(f.position!=at){if(std::fseek(f.file,long(at),SEEK_SET)!=0){f.position=~0ull;return MA_ERROR;}f.position=at;}
    s.scratch.resize(size_t(n*s.channels));
    const size_t got=std::fread(s.scratch.data(),s.channels*2,size_t(n),f.file);f.position+=u64(got)*s.channels*2;
    auto* pcm=static_cast<float*>(out);for(size_t i=0;i<got*s.channels;++i)pcm[i]=float(s.scratch[i])/32768.f;
    // Report MA_AT_END together with the final frames: miniaudio's looping
    // treats a later empty MA_AT_END read as "no audio" and stops the loop.
    s.cursor+=got;if(read)*read=got;return got&&s.cursor<s.frames?MA_SUCCESS:MA_AT_END;
}
ma_result pcm_seek(ma_data_source* source,ma_uint64 frame){
    auto& s=*static_cast<PcmStream*>(source);if(frame>s.frames)return MA_INVALID_ARGS;s.cursor=frame;return MA_SUCCESS;
}
ma_result pcm_format(ma_data_source* source,ma_format* format,ma_uint32* channels,ma_uint32* rate,ma_channel* map,size_t cap){
    auto& s=*static_cast<PcmStream*>(source);if(format)*format=ma_format_f32;if(channels)*channels=s.channels;if(rate)*rate=s.rate;
    if(map)ma_channel_map_init_standard(ma_standard_channel_map_default,map,cap,s.channels);return MA_SUCCESS;
}
ma_result pcm_cursor(ma_data_source* source,ma_uint64* cursor){*cursor=static_cast<PcmStream*>(source)->cursor;return MA_SUCCESS;}
ma_result pcm_length(ma_data_source* source,ma_uint64* length){*length=static_cast<PcmStream*>(source)->frames;return MA_SUCCESS;}
}
const ma_data_source_vtable pcm_vtable{pcm_read,pcm_seek,pcm_format,pcm_cursor,pcm_length,nullptr,0};
bool locate_music(std::FILE* f,u64 base,u32 size,i16 cue,MusicTrack& out,std::string& error){
    const auto le32=[](const u8* p){return u32(p[0])|u32(p[1])<<8|u32(p[2])<<16|u32(p[3])<<24;};
    const auto read=[&](u64 at,void* to,u32 n){return std::fseek(f,long(base+at),SEEK_SET)==0&&std::fread(to,1,n,f)==n;};
    u8 head[12];if(size<44||!read(0,head,12)||std::memcmp(head,"RIFF",4)||std::memcmp(head+8,"WAVE",4)){error="not a WAV file";return false;}
    u32 format=0,bits=0,data_bytes=0;u64 data=0;out={};
    for(u64 at=12;at+8<=size;){u8 chunk[24];if(!read(at,chunk,8)){error="unreadable";return false;}const u32 length=le32(chunk+4);
        if(!std::memcmp(chunk,"fmt ",4)&&length>=16&&read(at+8,chunk+8,16)){format=u32(chunk[8])|u32(chunk[9])<<8;out.channels=u32(chunk[10])|u32(chunk[11])<<8;out.rate=le32(chunk+12);bits=u32(chunk[22])|u32(chunk[23])<<8;}
        else if(!std::memcmp(chunk,"data",4)){data=at+8;data_bytes=u32(std::min<u64>(length,size-data));break;}
        at+=8+u64(length)+(length&1);}
    if(format!=1||bits!=16||!out.channels||out.channels>2||!out.rate||!data){error="unsupported WAV format (16-bit PCM expected)";return false;}
    const u32 total=data_bytes/(out.channels*2);u32 start=0,length=0;
    std::vector<u8> tail(std::min<u32>(320,size));if(!read(size-tail.size(),tail.data(),u32(tail.size()))){error="unreadable";return false;}
    for(size_t n=0;n+19<tail.size();n++){if(!std::memcmp(&tail[n],"cue",3))start=le32(&tail[n+16]);else if(!std::memcmp(&tail[n],"adtl",4))length=le32(&tail[n+16]);}
    if(cue==59){start=(0x60e8ac-0x5e0d90)/4;length=0x5e0d90/4;}
    out.offset=base+data;out.start=start;out.end=start&&length?start+length:total;out.loop=cue!=41;
    if(out.start>=out.end||out.end>total){error="loop outside the track";return false;}
    return true;
}
}
