#include "raop/fifo.h"

#include <gtest/gtest.h>

#include <numeric>
#include <thread>
#include <vector>

TEST(ConcurrentByteFIFOTest, BasicWriteAndRead) {
  ConcurrentByteFIFO fifo(8);

  std::vector<uint8_t> input = {1, 2, 3, 4};
  std::vector<uint8_t> output(4);

  EXPECT_EQ(fifo.Write(input.data(), input.size()), 4);
  EXPECT_FALSE(fifo.Empty());

  EXPECT_EQ(fifo.Read(output.data(), output.size()), 4);
  EXPECT_TRUE(fifo.Empty());
  EXPECT_EQ(input, output);
}

TEST(ConcurrentByteFIFOTest, WriteBlocksWhenFull) {
  ConcurrentByteFIFO fifo(4);

  std::vector<uint8_t> input = {1, 2, 3, 4};
  EXPECT_EQ(fifo.Write(input.data(), input.size()), 4);
  EXPECT_TRUE(fifo.Full());

  std::thread reader([&fifo]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    uint8_t temp[4];
    fifo.Read(temp, 4);
  });

  std::thread writer([&fifo]() {
    std::vector<uint8_t> data = {5, 6, 7, 8};
    // 会阻塞直到 reader 释放空间
    fifo.Write(data.data(), data.size());
  });

  reader.join();
  writer.join();

  EXPECT_TRUE(fifo.Full());
}

TEST(ConcurrentByteFIFOTest, ReadBlocksWhenEmpty) {
  ConcurrentByteFIFO fifo(4);

  std::thread writer([&fifo]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    uint8_t data[4] = {10, 11, 12, 13};
    fifo.Write(data, 4);
  });

  std::vector<uint8_t> output(4);
  size_t read = fifo.Read(output.data(), 4);

  writer.join();
  EXPECT_EQ(read, 4);
  EXPECT_EQ(output[0], 10);
  EXPECT_EQ(output[3], 13);
}

TEST(ConcurrentByteFIFOTest, ConcurrentReadWrite) {
  ConcurrentByteFIFO fifo(16);
  std::vector<uint8_t> write_data(1000);
  std::iota(write_data.begin(), write_data.end(), 0);  // 填充 0~999

  std::vector<uint8_t> read_data(1000);

  std::thread writer([&]() {
    size_t offset = 0;
    while (offset < write_data.size()) {
      size_t chunk = std::min<size_t>(8, write_data.size() - offset);
      offset += fifo.Write(&write_data[offset], chunk);
    }
  });

  std::thread reader([&]() {
    size_t offset = 0;
    while (offset < read_data.size()) {
      size_t chunk = std::min<size_t>(8, read_data.size() - offset);
      offset += fifo.Read(&read_data[offset], chunk);
    }
  });

  writer.join();
  reader.join();

  EXPECT_EQ(write_data, read_data);
}