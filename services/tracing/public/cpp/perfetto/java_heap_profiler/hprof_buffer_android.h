// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_PUBLIC_CPP_PERFETTO_JAVA_HEAP_PROFILER_HPROF_BUFFER_ANDROID_H_
#define SERVICES_TRACING_PUBLIC_CPP_PERFETTO_JAVA_HEAP_PROFILER_HPROF_BUFFER_ANDROID_H_

#include <stddef.h>
#include <cstdint>

#include "base/component_export.h"
#include "base/macros.h"

namespace tracing {

// Helper class that has methods to help parse the hprof file data passed in.
// Works by accessing byte one at a time through data_[data_position_] while
// also incrementing |data_position_| after reading a byte.
class COMPONENT_EXPORT(TRACING_CPP) HprofBuffer {
 public:
  HprofBuffer(const unsigned char* data, size_t size);
  HprofBuffer(const HprofBuffer&) = delete;
  HprofBuffer& operator=(const HprofBuffer&) = delete;

  uint32_t GetOneByte();
  uint32_t GetTwoBytes();
  uint32_t GetFourBytes();
  uint64_t GetId();
  bool HasRemaining();
  void set_id_size(unsigned id_size);
  void set_position(size_t new_position);

  // Skips |delta| bytes in the buffer.
  void Skip(uint32_t delta);

 private:
  unsigned char GetByte();

  // Read in the next |num_bytes| as an uint32_t.
  uint32_t GetUInt32FromBytes(size_t num_bytes);

  // Read in the next |num_bytes| as an uint64_t.
  uint64_t GetUInt64FromBytes(size_t num_bytes);

  const unsigned char* const data_;
  const size_t size_;
  size_t data_position_ = 0;
  // The ID size in bytes of the objects in the hprof, valid values are 4 and 8.
  unsigned object_id_size_in_bytes_ = 4;
};

}  // namespace tracing

#endif  // SERVICES_TRACING_PUBLIC_CPP_PERFETTO_JAVA_HEAP_PROFILER_HPROF_BUFFER_ANDROID_H_
