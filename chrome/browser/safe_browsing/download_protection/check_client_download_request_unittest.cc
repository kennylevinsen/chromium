// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/download_protection/check_client_download_request.h"

#include <string>
#include <tuple>

#include "base/test/metrics/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

namespace {

constexpr BinaryUploadService::Result kAllBinaryUploadServiceResults[]{
    BinaryUploadService::Result::UNKNOWN,
    BinaryUploadService::Result::SUCCESS,
    BinaryUploadService::Result::UPLOAD_FAILURE,
    BinaryUploadService::Result::TIMEOUT,
    BinaryUploadService::Result::FILE_TOO_LARGE,
    BinaryUploadService::Result::FAILED_TO_GET_TOKEN};

constexpr int64_t kTotalBytes = 1000;

constexpr base::TimeDelta kDuration = base::TimeDelta::FromSeconds(10);

constexpr base::TimeDelta kInvalidDuration = base::TimeDelta::FromSeconds(0);

std::string ResultToString(const BinaryUploadService::Result& result,
                           bool success) {
  std::string result_value;
  switch (result) {
    case BinaryUploadService::Result::SUCCESS:
      if (success)
        result_value = "Success";
      else
        result_value = "FailedToGetVerdict";
      break;
    case BinaryUploadService::Result::UPLOAD_FAILURE:
      result_value = "UploadFailure";
      break;
    case BinaryUploadService::Result::TIMEOUT:
      result_value = "Timeout";
      break;
    case BinaryUploadService::Result::FILE_TOO_LARGE:
      result_value = "FileTooLarge";
      break;
    case BinaryUploadService::Result::FAILED_TO_GET_TOKEN:
      result_value = "FailedToGetToken";
      break;
    case BinaryUploadService::Result::UNKNOWN:
      result_value = "Unknown";
      break;
  }
  return result_value;
}

}  // namespace

class CheckClientDownloadRequestUMATest
    : public testing::TestWithParam<
          std::tuple<DeepScanAccessPoint, BinaryUploadService::Result>> {
 public:
  CheckClientDownloadRequestUMATest() {}

  DeepScanAccessPoint access_point() const { return std::get<0>(GetParam()); }

  std::string access_point_string() const {
    return DeepScanAccessPointToString(access_point());
  }

  BinaryUploadService::Result result() const { return std::get<1>(GetParam()); }

  bool success() const {
    return result() == BinaryUploadService::Result::SUCCESS;
  }

  std::string result_value(bool success) const {
    return ResultToString(result(), success);
  }

  const base::HistogramTester& histograms() const { return histograms_; }

 private:
  base::HistogramTester histograms_;
};

INSTANTIATE_TEST_SUITE_P(
    Tests,
    CheckClientDownloadRequestUMATest,
    testing::Combine(testing::Values(DeepScanAccessPoint::DOWNLOAD),
                     testing::ValuesIn(kAllBinaryUploadServiceResults)));

TEST_P(CheckClientDownloadRequestUMATest, SuccessfulScanVerdicts) {
  RecordDeepScanMetrics(access_point(), kDuration, kTotalBytes, result(),
                        DeepScanningClientResponse());
  if (success()) {
    histograms().ExpectUniqueSample(
        "SafeBrowsing.DeepScan." + access_point_string() + ".BytesPerSeconds",
        kTotalBytes / kDuration.InSeconds(), 1);
  }
  histograms().ExpectTimeBucketCount(
      "SafeBrowsing.DeepScan." + access_point_string() + ".Duration", kDuration,
      1);
  histograms().ExpectTimeBucketCount("SafeBrowsing.DeepScan." +
                                         access_point_string() + "." +
                                         result_value(true) + ".Duration",
                                     kDuration, 1);
}

TEST_P(CheckClientDownloadRequestUMATest, UnsuccessfulDlpScanVerdict) {
  DlpDeepScanningVerdict dlp_verdict;
  dlp_verdict.set_status(DlpDeepScanningVerdict::FAILURE);
  DeepScanningClientResponse response;
  *response.mutable_dlp_scan_verdict() = dlp_verdict;

  RecordDeepScanMetrics(access_point(), kDuration, kTotalBytes, result(),
                        response);

  histograms().ExpectTimeBucketCount(
      "SafeBrowsing.DeepScan." + access_point_string() + ".Duration", kDuration,
      1);
  histograms().ExpectTimeBucketCount("SafeBrowsing.DeepScan." +
                                         access_point_string() + "." +
                                         result_value(false) + ".Duration",
                                     kDuration, 1);
}

TEST_P(CheckClientDownloadRequestUMATest, UnsuccessfulMalwareScanVerdict) {
  MalwareDeepScanningVerdict malware_verdict;
  malware_verdict.set_verdict(MalwareDeepScanningVerdict::VERDICT_UNSPECIFIED);
  DeepScanningClientResponse response;
  *response.mutable_malware_scan_verdict() = malware_verdict;

  RecordDeepScanMetrics(access_point(), kDuration, kTotalBytes, result(),
                        response);

  histograms().ExpectTimeBucketCount(
      "SafeBrowsing.DeepScan." + access_point_string() + ".Duration", kDuration,
      1);
  histograms().ExpectTimeBucketCount("SafeBrowsing.DeepScan." +
                                         access_point_string() + "." +
                                         result_value(false) + ".Duration",
                                     kDuration, 1);
}

TEST_P(CheckClientDownloadRequestUMATest, BypassScanVerdict) {
  RecordDeepScanMetrics(access_point(), kDuration, kTotalBytes,
                        "BypassedByUser", true);

  histograms().ExpectTimeBucketCount(
      "SafeBrowsing.DeepScan." + access_point_string() + ".Duration", kDuration,
      1);
  histograms().ExpectTimeBucketCount("SafeBrowsing.DeepScan." +
                                         access_point_string() +
                                         ".BypassedByUser.Duration",
                                     kDuration, 1);
}

TEST_P(CheckClientDownloadRequestUMATest, InvalidDuration) {
  RecordDeepScanMetrics(access_point(), kInvalidDuration, kTotalBytes, result(),
                        DeepScanningClientResponse());
  EXPECT_EQ(
      0u,
      histograms().GetTotalCountsForPrefix("SafeBrowsing.DeepScan.").size());
}

}  // namespace safe_browsing
