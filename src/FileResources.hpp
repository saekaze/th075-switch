#pragma once
#include "../game/GameResources.hpp"
#include "../game/DataArchive.hpp"
#include "Platform.hpp"
#include <cstdio>
namespace th075::sdl {
// Switch port of th075_web/cpp/sdl/FileResources: the player's original
// archives are read in place instead of per-file browser downloads.
// ResourceEdition::Base is th075.dat; Patched is th075b.dat (the 1.11 patch)
// with th075.dat behind it, as upstream's patched -> base fallback. The
// upstream "/save/..." paths are the save folder on the SD card.
class FileResources final:public ResourceStore {
public:
 struct Archive {FILE* file=nullptr;DataArchive index;std::string name;u64 bytes=0;~Archive(){if(file)std::fclose(file);}};
 std::string error;
 bool open();   // th075.dat and th075b.dat from the data folder
 bool read(ResourceEdition,const std::string&,std::vector<u8>&)override;
 static bool open_archive(Archive&,const std::string& path,std::string& error);
 static bool read_entry(Archive&,const ArchiveEntry&,std::vector<u8>&);
 static bool chinese(){return false;}
 static std::string host_path(const std::string&);   // "/save/x" -> <save folder>/x
 static bool read_file(const std::string&,std::vector<u8>&,u32 limit=134217728);
 static bool write_file(const std::string&,const void*,u32);
private:
 Archive base,patch;
};
}
