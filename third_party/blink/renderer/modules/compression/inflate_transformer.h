// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COMPRESSION_INFLATE_TRANSFORMER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COMPRESSION_INFLATE_TRANSFORMER_H_

#include "base/util/type_safety/strong_alias.h"

#include "third_party/blink/renderer/core/streams/transform_stream_transformer.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "third_party/zlib/zlib.h"

namespace blink {

enum class CompressionFormat;

class InflateTransformer final : public TransformStreamTransformer {
 public:
  InflateTransformer(ScriptState*, CompressionFormat);
  ~InflateTransformer() override;

  ScriptPromise Transform(v8::Local<v8::Value> chunk,
                          TransformStreamDefaultControllerInterface*,
                          ExceptionState&) override;

  ScriptPromise Flush(TransformStreamDefaultControllerInterface*,
                      ExceptionState&) override;

  ScriptState* GetScriptState() override { return script_state_; }

  void Trace(Visitor*) override;

 private:
  using IsFinished = util::StrongAlias<class IsFinishedTag, bool>;

  void Inflate(const uint8_t*,
               wtf_size_t,
               IsFinished,
               TransformStreamDefaultControllerInterface*,
               ExceptionState&);

  Member<ScriptState> script_state_;

  z_stream stream_;

  Vector<uint8_t> out_buffer_;

  bool was_flush_called_ = false;

  // This buffer size has been experimentally verified to be optimal.
  static constexpr wtf_size_t kBufferSize = 65536;

  DISALLOW_COPY_AND_ASSIGN(InflateTransformer);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COMPRESSION_INFLATE_TRANSFORMER_H_
