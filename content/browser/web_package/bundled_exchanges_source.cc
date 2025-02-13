// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/bundled_exchanges_source.h"

#include "base/files/file.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "net/base/filename_util.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "base/android/content_uri_utils.h"
#endif

namespace content {

// static
std::unique_ptr<BundledExchangesSource>
BundledExchangesSource::MaybeCreateFromTrustedFileUrl(const GURL& url) {
#if defined(OS_ANDROID)
  if (url.SchemeIs(url::kContentScheme)) {
    const base::FilePath file_path = base::FilePath(url.spec());
    return base::WrapUnique(
        new BundledExchangesSource(Type::kTrustedFile, file_path, url));
  }
#endif
  DCHECK(url.SchemeIsFile());
  base::FilePath file_path;
  if (!net::FileURLToFilePath(url, &file_path))
    return nullptr;
  return base::WrapUnique(
      new BundledExchangesSource(Type::kTrustedFile, file_path, url));
}

// static
std::unique_ptr<BundledExchangesSource>
BundledExchangesSource::MaybeCreateFromFileUrl(const GURL& url) {
  base::FilePath file_path;
  if (url.SchemeIsFile()) {
    if (net::FileURLToFilePath(url, &file_path)) {
      return base::WrapUnique(
          new BundledExchangesSource(Type::kFile, file_path, url));
    }
  }
#if defined(OS_ANDROID)
  if (url.SchemeIs(url::kContentScheme)) {
    return base::WrapUnique(new BundledExchangesSource(
        Type::kFile, base::FilePath(url.spec()), url));
  }
#endif
  return nullptr;
}

// static
std::unique_ptr<BundledExchangesSource>
BundledExchangesSource::MaybeCreateFromNetworkUrl(const GURL& url) {
  if (url.SchemeIs(url::kHttpsScheme) ||
      (url.SchemeIs(url::kHttpScheme) && net::IsLocalhost(url))) {
    return base::WrapUnique(
        new BundledExchangesSource(Type::kNetwork, base::FilePath(), url));
  }
  return nullptr;
}

std::unique_ptr<BundledExchangesSource> BundledExchangesSource::Clone() const {
  return base::WrapUnique(new BundledExchangesSource(type_, file_path_, url_));
}

std::unique_ptr<base::File> BundledExchangesSource::OpenFile() const {
  DCHECK(!file_path_.empty());
#if defined(OS_ANDROID)
  if (file_path_.IsContentUri()) {
    return std::make_unique<base::File>(
        base::OpenContentUriForRead(file_path_));
  }
#endif
  return std::make_unique<base::File>(
      file_path_, base::File::FLAG_OPEN | base::File::FLAG_READ);
}

BundledExchangesSource::BundledExchangesSource(Type type,
                                               const base::FilePath& file_path,
                                               const GURL& url)
    : type_(type), file_path_(file_path), url_(url) {}

}  // namespace content
