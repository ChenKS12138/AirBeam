#include <fstream>
#include <ios>
#include <iostream>
#include <thread>

#include "raop/raop.h"

using namespace AirBeamCore::raop;

int main() {
  std::cout << "Begin App" << std::endl;

  Raop raop("192.168.123.109");
  raop.Start();

  std::ifstream ifs(
      "/Users/cattchen/Codes/github.com/ChenKS12138/AirBeam/images/audio.pcm",
      std::ios::binary);

  if (ifs.is_open()) {
    ifs.seekg(0, std::ios::end);
    size_t length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    char* buffer = new char[length];

    std::cout << "Reading " << length << " characters... " << std::endl;
    ifs.read(buffer, length);

    if (ifs) {
      std::cout << "all characters read successfully." << std::endl;
    } else {
      std::cout << "error: only " << ifs.gcount() << " could be read";
    }
    ifs.close();

    // TODO: Send buffer to raop
    delete[] buffer;
  } else {
    std::cout << "Unable to open file";
  }

  return 0;
}