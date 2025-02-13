// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERIVCES_PROXY_PROXY_RESOLVER_V8_TRACING_H_
#define SERIVCES_PROXY_PROXY_RESOLVER_V8_TRACING_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/base/completion_once_callback.h"
#include "net/proxy_resolution/proxy_resolver.h"
#include "net/proxy_resolution/proxy_resolver_factory.h"

namespace net {
class NetLogWithSource;
class NetworkIsolationKey;
}  // namespace net

namespace proxy_resolver {

class ProxyHostResolver;

// ProxyResolverV8Tracing is a non-blocking proxy resolver.
class ProxyResolverV8Tracing {
 public:
  // Bindings is an interface used by ProxyResolverV8Tracing to delegate
  // per-request functionality. Each instance will be destroyed on the origin
  // thread of the ProxyResolverV8Tracing when the request completes or is
  // cancelled. All methods will be invoked from the origin thread.
  class Bindings {
   public:
    Bindings() {}
    virtual ~Bindings() {}

    // Invoked in response to an alert() call by the PAC script.
    virtual void Alert(const base::string16& message) = 0;

    // Invoked in response to an error in the PAC script.
    virtual void OnError(int line_number, const base::string16& message) = 0;

    // Returns a HostResolver to use for DNS resolution.
    virtual ProxyHostResolver* GetHostResolver() = 0;

    // Returns a NetLogWithSource to be passed to the HostResolver returned by
    // GetHostResolver().
    virtual net::NetLogWithSource GetNetLogWithSource() = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Bindings);
  };

  virtual ~ProxyResolverV8Tracing() {}

  // Gets a list of proxy servers to use for |url|. This request always
  // runs asynchronously and notifies the result by running |callback|. If the
  // result code is OK then the request was successful and |results| contains
  // the proxy resolution information.  Request can be cancelled by resetting
  // |*request|.
  virtual void GetProxyForURL(
      const GURL& url,
      const net::NetworkIsolationKey& network_isolation_key,
      net::ProxyInfo* results,
      net::CompletionOnceCallback callback,
      std::unique_ptr<net::ProxyResolver::Request>* request,
      std::unique_ptr<Bindings> bindings) = 0;
};

// A factory for ProxyResolverV8Tracing instances. The default implementation,
// returned by Create(), creates ProxyResolverV8Tracing instances that execute
// ProxyResolverV8 on a single helper thread, and do some magic to avoid
// blocking in DNS. For more details see the design document:
// https://docs.google.com/a/google.com/document/d/16Ij5OcVnR3s0MH4Z5XkhI9VTPoMJdaBn9rKreAmGOdE/edit?pli=1
class ProxyResolverV8TracingFactory {
 public:
  ProxyResolverV8TracingFactory() {}
  virtual ~ProxyResolverV8TracingFactory() = default;

  virtual void CreateProxyResolverV8Tracing(
      const scoped_refptr<net::PacFileData>& pac_script,
      std::unique_ptr<ProxyResolverV8Tracing::Bindings> bindings,
      std::unique_ptr<ProxyResolverV8Tracing>* resolver,
      net::CompletionOnceCallback callback,
      std::unique_ptr<net::ProxyResolverFactory::Request>* request) = 0;

  static std::unique_ptr<ProxyResolverV8TracingFactory> Create();

 private:
  DISALLOW_COPY_AND_ASSIGN(ProxyResolverV8TracingFactory);
};

}  // namespace proxy_resolver

#endif  // SERIVCES_PROXY_PROXY_RESOLVER_V8_TRACING_H_
