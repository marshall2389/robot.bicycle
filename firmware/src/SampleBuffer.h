#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

#include <array>
#include <cstdint>

#include "ch.h"
#include "ff.h"
#include "sample.pb.h"
#include "Singleton.h"

class SampleBuffer : public Singleton<SampleBuffer> {
  friend class Singleton<SampleBuffer>;
 public:
  void initialize(const char * filename);
  msg_t insert(Sample & s);
  msg_t deinitialize();

 private:
  SampleBuffer();
  SampleBuffer(const SampleBuffer &) = delete;
  SampleBuffer & operator=(const SampleBuffer &) = delete;

  static const uint16_t bytes_per_block_ = 512;
  static const uint16_t blocks_per_buffer_ = 32;
  static const uint16_t bytes_per_buffer_ = bytes_per_block_ * blocks_per_buffer_;
  static const uint16_t buffer_cushion_bytes_ = 256;
  static const uint8_t number_of_buffers_ = 3;

  std::array<std::array<uint8_t, bytes_per_buffer_ + buffer_cushion_bytes_>,
             number_of_buffers_> buffer_;

  static msg_t manager_thread_(void * arg);
  msg_t manager_thread(void * arg);

  static msg_t write_thread_(void * arg);
  msg_t write_thread(void * arg);
  WORKING_AREA(waWriteThread, 4096);
  WORKING_AREA(waManagementThread, 1024);

  FIL f_;
  Thread * tp_write_, * tp_manage_;
  Mutex buffer_mtx_;
  uint8_t active_buffer_;
};

#include "SampleBuffer_priv.h"

#endif // SAMPLEBUFFER_H
