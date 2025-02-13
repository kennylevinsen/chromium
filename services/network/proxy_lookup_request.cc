// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/proxy_lookup_request.h"

#include <string>

#include "base/bind.h"
#include "base/optional.h"
#include "net/base/net_errors.h"
#include "net/base/network_isolation_key.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/log/net_log_with_source.h"
#include "net/url_request/url_request_context.h"
#include "services/network/network_context.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "url/gurl.h"

namespace network {

ProxyLookupRequest::ProxyLookupRequest(
    mojo::PendingRemote<mojom::ProxyLookupClient> proxy_lookup_client,
    NetworkContext* network_context)
    : network_context_(network_context),
      proxy_lookup_client_(std::move(proxy_lookup_client)) {
  DCHECK(proxy_lookup_client_);
}

ProxyLookupRequest::~ProxyLookupRequest() {
  // |request_| should be non-null only when the network service is being torn
  // down.
  if (request_)
    proxy_lookup_client_->OnProxyLookupComplete(net::ERR_ABORTED,
                                                base::nullopt);
}

void ProxyLookupRequest::Start(const GURL& url) {
  proxy_lookup_client_.set_disconnect_handler(
      base::BindOnce(&ProxyLookupRequest::DestroySelf, base::Unretained(this)));
  // TODO(mmenke): The NetLogWithSource() means nothing is logged. Fix that.
  //
  // TODO(https://crbug.com/1023435): Pass along a NetworkIsolationKey.
  int result =
      network_context_->url_request_context()
          ->proxy_resolution_service()
          ->ResolveProxy(url, std::string(), net::NetworkIsolationKey::Todo(),
                         &proxy_info_,
                         base::BindOnce(&ProxyLookupRequest::OnResolveComplete,
                                        base::Unretained(this)),
                         &request_, net::NetLogWithSource());
  if (result != net::ERR_IO_PENDING)
    OnResolveComplete(result);
}

void ProxyLookupRequest::OnResolveComplete(int result) {
  if (result == net::OK) {
    proxy_lookup_client_->OnProxyLookupComplete(
        net::OK, base::Optional<net::ProxyInfo>(std::move(proxy_info_)));
  } else {
    proxy_lookup_client_->OnProxyLookupComplete(result, base::nullopt);
  }
  DestroySelf();
}

void ProxyLookupRequest::DestroySelf() {
  request_.reset();
  network_context_->OnProxyLookupComplete(this);
}

}  // namespace network
