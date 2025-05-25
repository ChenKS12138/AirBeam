#include <atomic>
#include <fstream>
#include <ios>
#include <iostream>
#include <latch>
#include <thread>

#include "raop/codec.h"
#include "raop/fifo.h"
#include "raop/raop.h"

using namespace AirBeamCore::raop;

int main() {
  std::cout << "Begin App" << std::endl;

  Raop raop("192.168.123.109");
  raop.Start();
  raop.SetVolume(30);

  std::cout << "Connected" << std::endl;

  constexpr size_t kFiFOCapacity = 5 * 1024 * 1024;  // 30MB buffer

  ConcurrentByteFIFO fifo(kFiFOCapacity);
  std::ifstream ifs(
      "/Users/cattchen/Codes/github.com/ChenKS12138/AirBeam/images/audio.pcm",
      std::ios::binary);

  if (!ifs.is_open()) {
    std::cerr << "Failed to open file." << std::endl;
    return 1;
  }

  std::atomic<bool> write_done = false;

  std::thread worker_thread([&]() {
    char chunk[1024];
    while (ifs.read(chunk, sizeof(chunk)) || ifs.gcount() > 0) {
      fifo.Write(reinterpret_cast<uint8_t*>(chunk), ifs.gcount());
    }

    if (ifs.eof()) {
      std::cout << "Finished reading file." << std::endl;
    } else if (ifs.fail()) {
      std::cerr << "Error reading file." << std::endl;
    }

    ifs.close();
    write_done.store(true);
  });
  worker_thread.detach();

  RtpAudioPacketChunk chunk, encoded;
  while (!write_done || !fifo.Empty()) {
    size_t size = fifo.Read(chunk.data_, sizeof(chunk.data_));
    chunk.len_ = size;
    PCMCodec::Encode(chunk, encoded);
    encoded.len_ = chunk.len_;
    raop.AcceptFrame();
    raop.SendChunk(encoded);
  }

  return 0;
}