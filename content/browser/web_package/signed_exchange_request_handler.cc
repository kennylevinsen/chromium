// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_request_handler.h"

#include <memory>

#include "base/bind.h"
#include "base/feature_list.h"
#include "content/browser/loader/single_request_url_loader_factory.h"
#include "content/browser/web_package/signed_exchange_devtools_proxy.h"
#include "content/browser/web_package/signed_exchange_loader.h"
#include "content/browser/web_package/signed_exchange_prefetch_metric_recorder.h"
#include "content/browser/web_package/signed_exchange_reporter.h"
#include "content/browser/web_package/signed_exchange_utils.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "third_party/blink/public/common/loader/throttling_url_loader.h"
#include "third_party/blink/public/common/origin_trials/trial_token_validator.h"

namespace content {

// static
bool SignedExchangeRequestHandler::IsSupportedMimeType(
    const std::string& mime_type) {
  return mime_type == "application/signed-exchange";
}

SignedExchangeRequestHandler::SignedExchangeRequestHandler(
    uint32_t url_loader_options,
    int frame_tree_node_id,
    const base::UnguessableToken& devtools_navigation_token,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    URLLoaderThrottlesGetter url_loader_throttles_getter,
    scoped_refptr<SignedExchangePrefetchMetricRecorder> metric_recorder,
    std::string accept_langs)
    : url_loader_options_(url_loader_options),
      frame_tree_node_id_(frame_tree_node_id),
      devtools_navigation_token_(devtools_navigation_token),
      url_loader_factory_(url_loader_factory),
      url_loader_throttles_getter_(std::move(url_loader_throttles_getter)),
      metric_recorder_(std::move(metric_recorder)),
      accept_langs_(std::move(accept_langs)) {}

SignedExchangeRequestHandler::~SignedExchangeRequestHandler() = default;

void SignedExchangeRequestHandler::MaybeCreateLoader(
    const network::ResourceRequest& tentative_resource_request,
    BrowserContext* browser_context,
    LoaderCallback callback,
    FallbackCallback fallback_callback) {
  if (!signed_exchange_loader_) {
    std::move(callback).Run({});
    return;
  }

  if (signed_exchange_loader_->fallback_url()) {
    DCHECK(tentative_resource_request.url.EqualsIgnoringRef(
        *signed_exchange_loader_->fallback_url()));
    signed_exchange_loader_ = nullptr;
    std::move(fallback_callback)
        .Run(false /* reset_subresource_loader_params */);
    return;
  }

  DCHECK(tentative_resource_request.url.EqualsIgnoringRef(
      *signed_exchange_loader_->inner_request_url()));
  std::move(callback).Run(base::MakeRefCounted<SingleRequestURLLoaderFactory>(
      base::BindOnce(&SignedExchangeRequestHandler::StartResponse,
                     weak_factory_.GetWeakPtr())));
}

bool SignedExchangeRequestHandler::MaybeCreateLoaderForResponse(
    const network::ResourceRequest& request,
    const network::ResourceResponseHead& response_head,
    mojo::ScopedDataPipeConsumerHandle* response_body,
    network::mojom::URLLoaderPtr* loader,
    mojo::PendingReceiver<network::mojom::URLLoaderClient>* client_receiver,
    blink::ThrottlingURLLoader* url_loader,
    bool* skip_other_interceptors,
    bool* will_return_unsafe_redirect) {
  DCHECK(!signed_exchange_loader_);
  if (!signed_exchange_utils::ShouldHandleAsSignedHTTPExchange(request.url,
                                                               response_head)) {
    return false;
  }

  mojo::PendingRemote<network::mojom::URLLoaderClient> client;
  *client_receiver = client.InitWithNewPipeAndPassReceiver();

  // This lets the SignedExchangeLoader directly returns an artificial redirect
  // to the downstream client without going through blink::ThrottlingURLLoader,
  // which means some checks like SafeBrowsing may not see the redirect. Given
  // that the redirected request will be checked when it's restarted we suppose
  // this is fine.
  signed_exchange_loader_ = std::make_unique<SignedExchangeLoader>(
      request, response_head, std::move(*response_body), std::move(client),
      url_loader->Unbind(), url_loader_options_,
      true /* should_redirect_to_fallback */,
      std::make_unique<SignedExchangeDevToolsProxy>(
          request.url, response_head, frame_tree_node_id_,
          devtools_navigation_token_, request.report_raw_headers),
      SignedExchangeReporter::MaybeCreate(request.url, request.referrer.spec(),
                                          response_head, frame_tree_node_id_),
      url_loader_factory_, url_loader_throttles_getter_, frame_tree_node_id_,
      metric_recorder_, accept_langs_);

  *skip_other_interceptors = true;
  return true;
}

void SignedExchangeRequestHandler::StartResponse(
    const network::ResourceRequest& resource_request,
    mojo::PendingReceiver<network::mojom::URLLoader> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
  signed_exchange_loader_->ConnectToClient(std::move(client));
  mojo::MakeSelfOwnedReceiver(std::move(signed_exchange_loader_),
                              std::move(receiver));
}

}  // namespace content
