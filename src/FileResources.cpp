#include "FileResources.hpp"
#include "Platform.hpp"
namespace th075::sdl {
std::string FileResources::host_path(const std::string& path){return path.rfind("/save",0)==0?host::paths().save+path.substr(5):path;}
bool FileResources::read_file(const std::string& path,std::vector<u8>& data,u32 limit){return host::read_file(host_path(path),data,limit);}
bool FileResources::write_file(const std::string& path,const void* bytes,u32 size){return host::write_file(host_path(path),static_cast<const u8*>(bytes),size);}
bool FileResources::open_archive(Archive& a,const std::string& path,std::string& error){
 a=Archive{};a.name=path;a.file=std::fopen(path.c_str(),"rb");if(!a.file){error="Missing "+path;return false;}
 std::fseek(a.file,0,SEEK_END);const long size=std::ftell(a.file);std::fseek(a.file,0,SEEK_SET);if(size<2){error="Invalid "+path;return false;}a.bytes=u64(size);
 u8 head[2];if(std::fread(head,1,2,a.file)!=2){error="Unable to read "+path;return false;}
 const u32 count=u32(head[0])|u32(head[1])<<8;std::vector<u8> index(2+std::size_t(count)*108);index[0]=head[0];index[1]=head[1];
 if(std::fread(index.data()+2,1,index.size()-2,a.file)!=index.size()-2||!a.index.read_index(index.data(),index.size(),a.bytes)){error=path+": "+(a.index.error.empty()?"truncated index":a.index.error);return false;}
 host::log("%s: %u entries",path.c_str(),count);return true;
}
bool FileResources::read_entry(Archive& a,const ArchiveEntry& e,std::vector<u8>& out){
 out.resize(e.size);return std::fseek(a.file,long(e.offset),SEEK_SET)==0&&(e.size==0||std::fread(out.data(),1,e.size,a.file)==e.size);
}
bool FileResources::open(){
 if(!open_archive(base,host::paths().data_file("th075.dat"),error))return false;
 // th075b.dat is part of the 1.11 update; without it only patched files fall back to 1.00 data.
 std::string missing;if(!open_archive(patch,host::paths().data_file("th075b.dat"),missing)){host::log("%s (1.11 patch data unavailable)",missing.c_str());patch=Archive{};}
 return true;
}
bool FileResources::read(ResourceEdition edition,const std::string& resource,std::vector<u8>& out){
 std::string path=resource;for(auto& c:path)if(c==char(92))c='/';
 if(path.empty()||path[0]=='/'||path.find("..")!=std::string::npos||path.find(':')!=std::string::npos){error="Invalid resource path";return false;}
 if(edition==ResourceEdition::Patched&&patch.file)if(const auto* e=patch.index.find(path)){if(read_entry(patch,*e,out))return true;error="Unable to read "+path+" from th075b.dat";return false;}
 if(const auto* e=base.file?base.index.find(path):nullptr){if(read_entry(base,*e,out))return true;error="Unable to read "+path+" from th075.dat";return false;}
 error="Missing resource: "+path;return false;
}
}
