// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/test/test_timeouts.h"
#include "build/build_config.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/render_text.h"

namespace {

#if defined(OS_WIN)
const char kFontDescription[] = "Segoe UI, 13px";
#elif defined(OS_ANDROID)
const char kFontDescription[] = "serif, 13px";
#else
const char kFontDescription[] = "sans, 13px";
#endif

struct Environment {
  Environment()
      : task_environment((base::CommandLine::Init(0, nullptr),
                          TestTimeouts::Initialize(),
                          base::test::TaskEnvironment::MainThreadType::UI)) {
    logging::SetMinLogLevel(logging::LOG_FATAL);

    CHECK(base::i18n::InitializeICU());
    gfx::FontList::SetDefaultFontDescription(kFontDescription);
  }

  base::AtExitManager at_exit_manager;
  base::test::TaskEnvironment task_environment;
};

}  // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static Environment env;

  std::unique_ptr<gfx::RenderText> render_text =
      gfx::RenderText::CreateHarfBuzzInstance();
  gfx::Canvas canvas;
  render_text->SetText(base::UTF8ToUTF16(
      base::StringPiece(reinterpret_cast<const char*>(data), size)));
  render_text->Draw(&canvas);
  return 0;
}
