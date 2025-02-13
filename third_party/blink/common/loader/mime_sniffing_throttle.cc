// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/loader/mime_sniffing_throttle.h"

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/base/mime_sniffer.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/blink/public/common/loader/mime_sniffing_url_loader.h"

namespace blink {

MimeSniffingThrottle::MimeSniffingThrottle(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {}

MimeSniffingThrottle::~MimeSniffingThrottle() = default;

void MimeSniffingThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  // No need to do mime sniffing again.
  if (response_head->did_mime_sniff)
    return;

  bool blocked_sniffing_mime = false;
  std::string content_type_options;
  if (response_head->headers &&
      response_head->headers->GetNormalizedHeader("x-content-type-options",
                                                  &content_type_options)) {
    blocked_sniffing_mime =
        base::LowerCaseEqualsASCII(content_type_options, "nosniff");
  }

  if (!blocked_sniffing_mime &&
      net::ShouldSniffMimeType(response_url, response_head->mime_type)) {
    // Pause the response until the mime type becomes ready.
    *defer = true;

    mojo::PendingRemote<network::mojom::URLLoader> new_remote;
    mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
    network::mojom::URLLoaderPtr source_loader;
    mojo::PendingReceiver<network::mojom::URLLoaderClient>
        source_client_receiver;
    MimeSniffingURLLoader* mime_sniffing_loader;
    std::tie(new_remote, new_receiver, mime_sniffing_loader) =
        MimeSniffingURLLoader::CreateLoader(
            weak_factory_.GetWeakPtr(), response_url, response_head->Clone(),
            task_runner_);
    delegate_->InterceptResponse(
        network::mojom::URLLoaderPtr(std::move(new_remote)),
        std::move(new_receiver), &source_loader, &source_client_receiver);
    mime_sniffing_loader->Start(source_loader.PassInterface(),
                                std::move(source_client_receiver));
  }
}

void MimeSniffingThrottle::ResumeWithNewResponseHead(
    network::mojom::URLResponseHeadPtr new_response_head) {
  delegate_->UpdateDeferredResponseHead(std::move(new_response_head));
  delegate_->Resume();
}

}  // namespace blink
