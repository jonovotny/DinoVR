#include "webupdatereader.h"


WebUpdateReader::WebUpdateReader(std::string filename, UpdateChecker* uc){
  //stream = std::ifstream( filename.c_str());
  //std::ifstream s (filename.c_str());
  stream.open(filename);
  

  std::string line;
  std::cout << "SPAMSPAMSPAM" << std::endl;
  while (std::getline(stream, line)){
    //Eh, just need to cycle
    std::cout << line << std::endl;
  }

  callback = uc;

}

void WebUpdateReader::checkForUpdates(){
  if (stream.eof()){
    stream.clear();
  }
  std::string line;
  while(std::getline(stream, line)){
    processLine(line);
    std::cout << line << std::endl;
  }
}

//Assumes well formed input, this is probably dumb.
void WebUpdateReader::processLine(std::string line){
  std::size_t split_pos = line.find_first_of("=");
  std::string first = line.substr(0, split_pos);
  std::string second = line.substr(split_pos+1);

  callback->HandleUpdate(first, second);

}
