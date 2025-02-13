// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/peerconnection/rtc_session_description_platform.h"

namespace blink {

scoped_refptr<RTCSessionDescriptionPlatform>
RTCSessionDescriptionPlatform::Create(const WebString& type,
                                      const WebString& sdp) {
  return base::AdoptRef(new RTCSessionDescriptionPlatform(type, sdp));
}

RTCSessionDescriptionPlatform::RTCSessionDescriptionPlatform(
    const WebString& type,
    const WebString& sdp)
    : type_(type), sdp_(sdp) {}

}  // namespace blink
