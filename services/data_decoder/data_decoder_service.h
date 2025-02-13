// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_DATA_DECODER_SERVICE_H_
#define SERVICES_DATA_DECODER_DATA_DECODER_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/data_decoder/public/mojom/bundled_exchanges_parser.mojom.h"
#include "services/data_decoder/public/mojom/data_decoder_service.mojom.h"
#include "services/data_decoder/public/mojom/image_decoder.mojom.h"
#include "services/data_decoder/public/mojom/json_parser.mojom.h"
#include "services/data_decoder/public/mojom/xml_parser.mojom.h"

#ifdef OS_CHROMEOS
#include "services/data_decoder/public/mojom/ble_scan_parser.mojom.h"
#endif  // OS_CHROMEOS

namespace data_decoder {

class DataDecoderService : public mojom::DataDecoderService {
 public:
  DataDecoderService();
  explicit DataDecoderService(
      mojo::PendingReceiver<mojom::DataDecoderService> receiver);
  ~DataDecoderService() override;

  // May be used to establish a latent DataDecoderService binding for this
  // instance. May only be called once, and only if this instance was default-
  // constructed.
  void BindReceiver(mojo::PendingReceiver<mojom::DataDecoderService> receiver);

  // Configures the service to drop ImageDecoder receivers instead of binding
  // them. Useful for tests simulating service failures.
  void SimulateImageDecoderCrashForTesting(bool drop) {
    drop_image_decoders_ = drop;
  }

  // Same as above but for JsonParser receivers.
  void SimulateJsonParserCrashForTesting(bool drop) {
    drop_json_parsers_ = drop;
  }

 private:
  // mojom::DataDecoderService implementation:
  void BindImageDecoder(
      mojo::PendingReceiver<mojom::ImageDecoder> receiver) override;
  void BindJsonParser(
      mojo::PendingReceiver<mojom::JsonParser> receiver) override;
  void BindXmlParser(mojo::PendingReceiver<mojom::XmlParser> receiver) override;
  void BindBundledExchangesParserFactory(
      mojo::PendingReceiver<mojom::BundledExchangesParserFactory> receiver)
      override;

#ifdef OS_CHROMEOS
  void BindBleScanParser(
      mojo::PendingReceiver<mojom::BleScanParser> receiver) override;
#endif  // OS_CHROMEOS

  // In-process instances (e.g. on iOS or in tests) may have multiple concurrent
  // remote DataDecoderService clients.
  mojo::ReceiverSet<mojom::DataDecoderService> receivers_;

  bool drop_image_decoders_ = false;
  bool drop_json_parsers_ = false;

  DISALLOW_COPY_AND_ASSIGN(DataDecoderService);
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_DATA_DECODER_SERVICE_H_
