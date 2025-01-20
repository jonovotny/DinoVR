#include "Config.h"

#include <iostream>
#include <fstream>

Config::Config(std::string fileName){
  ReadFile(fileName);
}


std::string Config::GetValue(std::string key, std::string fallback){
  return GetValueWithDefault(key, fallback);
}

std::string Config::GetValueWithDefault(std::string key, std::string fallback){
  auto it = entries.find(key);
  if (it == entries.end()){
    return fallback;
  }
  return it->second;
}

void Config::AddEntry(std::string key, std::string value){
  entries[key] = value;
}

void Config::ReadFile(std::string fileName){
  std::ifstream stream (fileName.c_str());
  if (!stream) { // Check if file opened successfully
      std::cerr << "Error opening file!" << std::endl;
      return ;
  }

  std::string line;
  std::string key, value;
  while(std::getline(stream, line)){
//if(line.size() >= 1  && !line.compare(0, 1, "#")){
      std::size_t split_pos = line.find_first_of("=");
      if (split_pos != std::string::npos){
        key = line.substr(0, split_pos);
        value = line.substr(split_pos+1);
        AddEntry(key, value);
      }
//    }
  }
} 

void Config::Print(){
  std::cout << "Printing a Config of " << entries.size() << " entries." << std::endl;
  for (auto it : entries){
    std::cout << "  " << it.first << ":" << it.second << std::endl;
  }
}
