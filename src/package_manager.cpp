#include "s/package_manager.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs=std::filesystem;

std::string read_contents(const fs::path& path){
  std::ifstream input(path,std::ios::binary);
  if(!input)throw std::runtime_error("Cannot read "+path.string());
  return {std::istreambuf_iterator<char>(input),{}};
}
void write_contents(const fs::path& path,const std::string& data){
  std::ofstream output(path,std::ios::binary|std::ios::trunc);
  if(!output)throw std::runtime_error("Cannot write "+path.string());
  output<<data;
  if(!output)throw std::runtime_error("Write failed: "+path.string());
}
bool valid_name(const std::string& name){
  if(name.empty()||!((name[0]>='A'&&name[0]<='Z')||(name[0]>='a'&&name[0]<='z')||name[0]=='_'))return false;
  return std::all_of(name.begin()+1,name.end(),[](unsigned char c){return std::isalnum(c)||c=='_';});
}
std::string checksum(const std::string& value){
  // FNV-1a detects accidental changes; it is NOT a cryptographic signature.
  std::uint64_t hash=14695981039346656037ULL;
  for(unsigned char ch:value){hash^=ch;hash*=1099511628211ULL;}
  std::ostringstream out;
  out<<std::hex<<std::setfill('0')<<std::setw(16)<<hash;
  return out.str();
}
std::map<std::string,std::string> installed(const fs::path& packages){
  std::map<std::string,std::string> found;
  if(!fs::exists(packages))return found;
  for(const auto& file:fs::directory_iterator(packages)){
    if(!file.is_regular_file()||file.path().extension()!=".se")continue;
    const std::string name=file.path().stem().string();
    if(!valid_name(name))throw std::runtime_error("Invalid package name: "+name);
    found[name]=checksum(read_contents(file.path()));
  }
  return found;
}
void save_lock(const fs::path& root,const std::map<std::string,std::string>& packages){
  std::string lock="# SE local package lock (FNV1a64; not a security checksum)\n";
  for(const auto& [name,hash]:packages)lock+=name+" "+hash+"\n";
  fs::create_directories(root/".se");
  write_contents(root/".se/se.pkg.lock",lock);
}
void verify(const fs::path& root,const fs::path& packages){
  const auto lock=root/".se/se.pkg.lock";
  if(!fs::is_regular_file(lock))throw std::runtime_error("Missing .se/se.pkg.lock; run se pkg init.");
  std::istringstream input(read_contents(lock));
  std::map<std::string,std::string> expected;
  for(std::string line;std::getline(input,line);){
    if(line.empty()||line[0]=='#')continue;
    std::istringstream fields(line);
    std::string name,hash,extra;
    if(!(fields>>name>>hash)||fields>>extra||!valid_name(name)||hash.size()!=16||!expected.emplace(name,hash).second)
      throw std::runtime_error("Invalid package lock entry: "+line);
  }
  const auto actual=installed(packages);
  if(actual!=expected)throw std::runtime_error("Package files do not match .se/se.pkg.lock.");
}
} // namespace

int se_package_command(int argc,char** argv){
  try{
    if(argc<3)throw std::runtime_error("Usage: se pkg init|add <file.se>|remove <name>|list|verify");
    const std::string action=argv[2];
    const auto root=fs::current_path();
    const auto packages=root/".se/packages";
    if(action=="init"&&argc==3){
      if(fs::exists(root/"se.pkg.toml"))throw std::runtime_error("se.pkg.toml already exists.");
      fs::create_directories(packages);
      std::string name=root.filename().string();
      if(!valid_name(name))name="se_project";
      write_contents(root/"se.pkg.toml","[package]\nname = \""+name+"\"\n\n# Local modules live in .se/packages/\n");
      save_lock(root,installed(packages));
      std::cout<<"Initialized SE package project.\n";
      return 0;
    }
    if(action=="add"&&argc==4){
      const fs::path source=argv[3];
      if(!fs::is_regular_file(source)||source.extension()!=".se")throw std::runtime_error("Provide a local .se source file.");
      const auto name=source.stem().string();
      if(!valid_name(name))throw std::runtime_error("Package names must be SE identifiers.");
      fs::create_directories(packages);
      const auto destination=packages/(name+".se");
      if(fs::exists(destination))throw std::runtime_error("Package already installed: "+name);
      fs::copy_file(source,destination,fs::copy_options::none);
      save_lock(root,installed(packages));
      std::cout<<"Installed local package "<<name<<".\n";
      return 0;
    }
    if(action=="remove"&&argc==4){
      const std::string name=argv[3];
      if(!valid_name(name))throw std::runtime_error("Invalid package name.");
      if(!fs::remove(packages/(name+".se")))throw std::runtime_error("Package not installed: "+name);
      save_lock(root,installed(packages));
      std::cout<<"Removed "<<name<<".\n";
      return 0;
    }
    if(action=="list"&&argc==3){
      for(const auto& [name,hash]:installed(packages))std::cout<<name<<" "<<hash<<"\n";
      return 0;
    }
    if(action=="verify"&&argc==3){
      verify(root,packages);
      std::cout<<"Local package lock verified.\n";
      return 0;
    }
    throw std::runtime_error("Usage: se pkg init|add <file.se>|remove <name>|list|verify");
  }catch(const std::exception& error){
    std::cerr<<"SE package error: "<<error.what()<<'\n';
    return 1;
  }
}
