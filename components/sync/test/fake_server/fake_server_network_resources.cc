// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/test/fake_server/fake_server_network_resources.h"

#include <string>

#include "base/threading/thread_task_runner_handle.h"
#include "components/sync/engine/net/http_post_provider_factory.h"
#include "components/sync/test/fake_server/fake_server.h"
#include "components/sync/test/fake_server/fake_server_http_post_provider.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace fake_server {

namespace {

std::unique_ptr<syncer::HttpPostProviderFactory>
CreateFakeServerHttpPostProviderFactoryHelper(
    const base::WeakPtr<FakeServer>& fake_server,
    scoped_refptr<base::SequencedTaskRunner> fake_server_task_runner,
    const std::string& user_agent,
    std::unique_ptr<network::SharedURLLoaderFactoryInfo>
        url_loader_factory_info,
    const syncer::NetworkTimeUpdateCallback& network_time_update_callback) {
  return std::make_unique<FakeServerHttpPostProviderFactory>(
      fake_server, fake_server_task_runner);
}

}  // namespace

syncer::CreateHttpPostProviderFactory CreateFakeServerHttpPostProviderFactory(
    const base::WeakPtr<FakeServer>& fake_server) {
  // TODO(treib): Switch to SequencedTaskRunnerHandler.
  return base::BindRepeating(&CreateFakeServerHttpPostProviderFactoryHelper,
                             fake_server, base::ThreadTaskRunnerHandle::Get());
}

}  // namespace fake_server
