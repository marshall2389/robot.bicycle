#ifndef SAMPLEREADER_H
#define SAMPLEREADER_H

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Sample.h"
#include "SampleConverted.h"

class SampleReader {
 public:
  SampleReader(const char *);
  std::vector<SampleConverted> Convert();

 private:
  static double encoderRate(uint32_t counts, double rad_counts_per_sec);
  std::vector<Sample> samples_;
  std::vector<SampleConverted> samplesConverted_;
  bool converted_;
};

#endif
