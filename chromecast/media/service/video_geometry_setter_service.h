// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_SERVICE_VIDEO_GEOMETRY_SETTER_SERVICE_H_
#define CHROMECAST_MEDIA_SERVICE_VIDEO_GEOMETRY_SETTER_SERVICE_H_

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "chromecast/media/service/mojom/video_geometry_setter.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace chromecast {
namespace media {

// This service runs and destructs on the sequence where it's constructed, but
// the public methods can be run on any sequence.
class VideoGeometrySetterService : public mojom::VideoGeometryChangeSubscriber,
                                   public mojom::VideoGeometrySetter {
 public:
  VideoGeometrySetterService();
  ~VideoGeometrySetterService() final;

  void GetVideoGeometryChangeSubscriber(
      mojo::PendingReceiver<mojom::VideoGeometryChangeSubscriber>
          pending_receiver);
  void GetVideoGeometrySetter(
      mojo::PendingReceiver<mojom::VideoGeometrySetter> pending_receiver);

 private:
  // mojom::VideoGeometryChangeSubscriber implementation.
  void SubscribeToVideoGeometryChange(
      const base::UnguessableToken& overlay_plane_id,
      mojo::PendingRemote<mojom::VideoGeometryChangeClient>
          client_pending_remote,
      SubscribeToVideoGeometryChangeCallback callback) override;
  // mojom::VideoGeometrySetter implementation.
  void SetVideoGeometry(
      const gfx::RectF& rect_f,
      gfx::OverlayTransform transform,
      const base::UnguessableToken& overlay_plane_id) override;

  void OnVideoGeometryChangeClientGone(
      const base::UnguessableToken overlay_plane_id);

  const scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::flat_map<base::UnguessableToken,
                 mojo::Remote<mojom::VideoGeometryChangeClient>>
      video_geometry_change_clients_;

  mojo::ReceiverSet<mojom::VideoGeometryChangeSubscriber>
      video_geometry_change_subscriber_receivers_;
  // TODO(guohuideng): change mojo::ReceiverSet to mojo::Receiver.
  mojo::ReceiverSet<mojom::VideoGeometrySetter>
      video_geometry_setter_receivers_;

  base::WeakPtrFactory<VideoGeometrySetterService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(VideoGeometrySetterService);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_SERVICE_VIDEO_GEOMETRY_SETTER_SERVICE_H_