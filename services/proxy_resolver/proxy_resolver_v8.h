// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERIVCES_PROXY_RESOLVER_PROXY_RESOLVER_V8_H_
#define SERIVCES_PROXY_RESOLVER_PROXY_RESOLVER_V8_H_

#include <stddef.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "net/proxy_resolution/proxy_resolve_dns_operation.h"

class GURL;

namespace net {
class ProxyInfo;
class PacFileData;
}  // namespace net

namespace proxy_resolver {

// A synchronous ProxyResolver-like that uses V8 to evaluate PAC scripts.
class ProxyResolverV8 {
 public:
  // Interface for the javascript bindings.
  class JSBindings {
   public:
    JSBindings() {}

    // Handler for "dnsResolve()", "dnsResolveEx()", "myIpAddress()",
    // "myIpAddressEx()". Returns true on success and fills |*output| with the
    // result. If |*terminate| is set to true, then the script execution will
    // be aborted. Note that termination may not happen right away.
    virtual bool ResolveDns(const std::string& host,
                            net::ProxyResolveDnsOperation op,
                            std::string* output,
                            bool* terminate) = 0;

    // Handler for "alert(message)"
    virtual void Alert(const base::string16& message) = 0;

    // Handler for when an error is encountered. |line_number| may be -1
    // if a line number is not applicable to this error.
    virtual void OnError(int line_number, const base::string16& error) = 0;

   protected:
    virtual ~JSBindings() {}
  };

  // Constructs a ProxyResolverV8.
  static int Create(const scoped_refptr<net::PacFileData>& script_data,
                    JSBindings* bindings,
                    std::unique_ptr<ProxyResolverV8>* resolver);

  ~ProxyResolverV8();

  int GetProxyForURL(const GURL& url,
                     net::ProxyInfo* results,
                     JSBindings* bindings);

  // Get total/used heap memory usage of all v8 instances used by the proxy
  // resolver.
  static size_t GetTotalHeapSize();
  static size_t GetUsedHeapSize();

 private:
  // Context holds the Javascript state for the PAC script.
  class Context;

  explicit ProxyResolverV8(std::unique_ptr<Context> context);

  std::unique_ptr<Context> context_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolverV8);
};

}  // namespace proxy_resolver

#endif  // SERIVCES_PROXY_RESOLVER_PROXY_RESOLVER_V8_H_
