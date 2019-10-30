#include <string> 
#include <iostream>
#include <fstream>

class UpdateChecker {
public:
  virtual void HandleUpdate(std::string key, std::string value) {};
  UpdateChecker() {};
};

class WebUpdateReader{
public:
  WebUpdateReader(std::string filename, UpdateChecker* uc);
  void checkForUpdates();
  
private:
  UpdateChecker* callback;
  std::ifstream stream;

  void processLine(std::string line);
};


