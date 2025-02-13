// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webgpu/gpu_adapter.h"

#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/modules/webgpu/gpu_device.h"
#include "third_party/blink/renderer/modules/webgpu/gpu_request_adapter_options.h"

namespace blink {

// static
GPUAdapter* GPUAdapter::Create(
    const String& name,
    uint32_t adapter_service_id,
    const WGPUDeviceProperties& properties,
    scoped_refptr<DawnControlClientHolder> dawn_control_client) {
  return MakeGarbageCollected<GPUAdapter>(name, adapter_service_id, properties,
                                          std::move(dawn_control_client));
}

GPUAdapter::GPUAdapter(
    const String& name,
    uint32_t adapter_service_id,
    const WGPUDeviceProperties& properties,
    scoped_refptr<DawnControlClientHolder> dawn_control_client)
    : DawnObjectBase(dawn_control_client), name_(name) {
  // TODO(jiawei.shao@intel.com): add GPUExtensions as a member of GPUAdapter
}

const String& GPUAdapter::name() const {
  return name_;
}

// TODO(jiawei.shao@intel.com) request device with adapter_server_id
ScriptPromise GPUAdapter::requestDevice(ScriptState* script_state,
                                        const GPUDeviceDescriptor* descriptor) {
  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise promise = resolver->Promise();

  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  GPUDevice* device = GPUDevice::Create(
      execution_context, GetDawnControlClient(), this, descriptor);

  resolver->Resolve(device);
  return promise;
}

}  // namespace blink
