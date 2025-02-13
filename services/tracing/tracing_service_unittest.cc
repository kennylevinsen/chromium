// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/tracing/public/cpp/perfetto/perfetto_config.h"
#include "services/tracing/public/mojom/perfetto_service.mojom.h"
#include "services/tracing/public/mojom/tracing_service.mojom.h"
#include "services/tracing/tracing_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tracing {

class TracingServiceTest : public testing::Test {
 public:
  TracingServiceTest() = default;
  ~TracingServiceTest() override = default;

 protected:
  mojom::TracingService* service() { return &service_; }

 private:
  base::test::TaskEnvironment task_environment_;
  TracingService service_;

  DISALLOW_COPY_AND_ASSIGN(TracingServiceTest);
};

class TestTracingClient : public mojom::TracingSessionClient {
 public:
  void StartTracing(mojom::TracingService* service,
                    base::OnceClosure on_tracing_enabled) {
    service->BindConsumerHost(mojo::MakeRequest(&consumer_host_));

    perfetto::TraceConfig perfetto_config =
        tracing::GetDefaultPerfettoConfig(base::trace_event::TraceConfig(""),
                                          /*privacy_filtering_enabled=*/false);

    mojo::PendingRemote<tracing::mojom::TracingSessionClient>
        tracing_session_client;
    binding_.Bind(tracing_session_client.InitWithNewPipeAndPassReceiver());

    consumer_host_->EnableTracing(
        mojo::MakeRequest(&tracing_session_host_),
        std::move(tracing_session_client), std::move(perfetto_config),
        tracing::mojom::TracingClientPriority::kUserInitiated);

    tracing_session_host_->RequestBufferUsage(
        base::BindOnce([](base::OnceClosure on_response, bool, float,
                          bool) { std::move(on_response).Run(); },
                       std::move(on_tracing_enabled)));
  }

  // tracing::mojom::TracingSessionClient implementation:
  void OnTracingEnabled() override {}
  void OnTracingDisabled() override {}

 private:
  mojom::ConsumerHostPtr consumer_host_;
  mojom::TracingSessionHostPtr tracing_session_host_;
  mojo::Binding<mojom::TracingSessionClient> binding_{this};
};

TEST_F(TracingServiceTest, TracingServiceInstantiate) {
  TestTracingClient tracing_client;

  base::RunLoop tracing_started;
  tracing_client.StartTracing(service(), tracing_started.QuitClosure());
  tracing_started.Run();
}

}  // namespace tracing
