load("//src/main/java/com/google/android/libraries/feed/buildtools/testing:android_local_test.bzl", "DEFAULT_ANDROID_LOCAL_TEST_MANIFEST")

licenses(["notice"])  # Apache 2

android_local_test(
    name = "SchedulerApiWrapperTest",
    size = "small",
    timeout = "moderate",
    srcs = ["SchedulerApiWrapperTest.java"],
    manifest_values = DEFAULT_ANDROID_LOCAL_TEST_MANIFEST,
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/host/scheduler",
        "//src/main/java/com/google/android/libraries/feed/api/internal/common",
        "//src/main/java/com/google/android/libraries/feed/common/concurrent/testing",
        "//src/main/java/com/google/android/libraries/feed/hostimpl/scheduler",
        "//third_party:robolectric",
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "@maven//:com_google_truth_truth",
        "@maven//:org_mockito_mockito_core",
        "@robolectric//bazel:android-all",
    ],
)
