// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_TEST_TEST_SHARED_URL_LOADER_FACTORY_H_
#define SERVICES_NETWORK_TEST_TEST_SHARED_URL_LOADER_FACTORY_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace net {
class TestURLRequestContext;
}

namespace network {

class NetworkContext;
class NetworkService;

// A helper class to create a full functioning SharedURLLoaderFactory. This is
// backed by a real URLLoader implementation. Use this in unittests which have a
// real IO thread and want to exercise the network stack.
// Note that Clone() can be used to receive a SharedURLLoaderFactory that works
// across threads.
class TestSharedURLLoaderFactory : public SharedURLLoaderFactory {
 public:
  explicit TestSharedURLLoaderFactory(NetworkService* network_service = nullptr,
                                      bool is_trusted = false);

  // URLLoaderFactory implementation:
  void CreateLoaderAndStart(mojo::PendingReceiver<mojom::URLLoader> loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& request,
                            mojo::PendingRemote<mojom::URLLoaderClient> client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojo::PendingReceiver<mojom::URLLoaderFactory> receiver) override;

  // SharedURLLoaderFactoryInfo implementation
  std::unique_ptr<SharedURLLoaderFactoryInfo> Clone() override;

  NetworkContext* network_context() { return network_context_.get(); }

 private:
  friend class base::RefCounted<TestSharedURLLoaderFactory>;
  ~TestSharedURLLoaderFactory() override;

  std::unique_ptr<net::TestURLRequestContext> url_request_context_;
  std::unique_ptr<NetworkContext> network_context_;
  mojo::Remote<mojom::URLLoaderFactory> url_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestSharedURLLoaderFactory);
};

}  // namespace network

#endif  // SERVICES_NETWORK_TEST_TEST_SHARED_URL_LOADER_FACTORY_H_
