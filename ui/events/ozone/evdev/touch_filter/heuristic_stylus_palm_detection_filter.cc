// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/touch_filter/heuristic_stylus_palm_detection_filter.h"

#include <linux/input.h>

namespace ui {

void HeuristicStylusPalmDetectionFilter::Filter(
    const std::vector<InProgressTouchEvdev>& touches,
    base::TimeTicks time,
    std::bitset<kNumTouchEvdevSlots>* slots_to_hold,
    std::bitset<kNumTouchEvdevSlots>* slots_to_suppress) {
  slots_to_hold->reset();
  slots_to_suppress->reset();
  base::TimeTicks latest_stylus_time =
      shared_palm_state_->latest_stylus_touch_time_;
  for (int i = 0; i < kNumTouchEvdevSlots; ++i) {
    const auto& touch = touches[i];
    if (touch.tool_code == BTN_TOOL_PEN) {
      // We detect BTN_TOOL_PEN whenever a pen is even hovering. This is
      // mutually exclusive with finger touches, which is what we're interested
      // in. So we update latest_time.
      shared_palm_state_->latest_stylus_touch_time_ = time;
      return;
    }
    if (!touch.touching) {
      stroke_length_[i] = 0;
      continue;
    }
    if (stroke_length_[i] == 0) {
      // new touch!
      touch_started_time_[i] = time;
    }
    stroke_length_[i]++;
    base::TimeDelta time_since_stylus_for_touch_start =
        touch_started_time_[i] - latest_stylus_time;
    if (time_since_stylus_for_touch_start < time_after_stylus_to_cancel_) {
      slots_to_suppress->set(i, 1);
    } else if (time_since_stylus_for_touch_start < time_after_stylus_to_hold_ &&
               stroke_length_[i] <= hold_stroke_count_) {
      slots_to_hold->set(i, 1);
    }
  }
}

HeuristicStylusPalmDetectionFilter::HeuristicStylusPalmDetectionFilter(
    SharedPalmDetectionFilterState* shared_palm_state,
    int hold_stroke_count,
    base::TimeDelta hold,
    base::TimeDelta cancel)
    : PalmDetectionFilter(shared_palm_state),
      hold_stroke_count_(hold_stroke_count),
      time_after_stylus_to_hold_(hold),
      time_after_stylus_to_cancel_(cancel) {
  touch_started_time_.resize(kNumTouchEvdevSlots, base::TimeTicks::UnixEpoch());
  stroke_length_.resize(kNumTouchEvdevSlots, 0);
  DCHECK(hold >= cancel) << "Expected hold time to be longer than cancel time.";
}

HeuristicStylusPalmDetectionFilter::~HeuristicStylusPalmDetectionFilter() {}

const char HeuristicStylusPalmDetectionFilter::kFilterName[] =
    "HeuristicStylusPalmDetectionFilter";

std::string HeuristicStylusPalmDetectionFilter::FilterNameForTesting() const {
  return kFilterName;
}

base::TimeDelta HeuristicStylusPalmDetectionFilter::HoldTime() const {
  return time_after_stylus_to_hold_;
}

base::TimeDelta HeuristicStylusPalmDetectionFilter::CancelTime() const {
  return time_after_stylus_to_cancel_;
}

}  // namespace ui
