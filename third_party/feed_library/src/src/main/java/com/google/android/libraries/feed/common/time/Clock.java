// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.common.time;


/**
 * Interface of SystemClock; real instances should just delegate the calls to the static methods,
 * while mock instances return values set manually; see {@link android.os.SystemClock}. In addition,
 * this interface also has an instance method for {@link System#currentTimeMillis}, so it can be
 * mocked if needed
 */
/*@DoNotMock("Use com.google.android.libraries.feed.common.time.testing.FakeClock instead")*/
public interface Clock {
  /** Number of nanoseconds in a single millisecond. */
  long NS_IN_MS = 1_000_000;

  /**
   * Returns the current system time in milliseconds since January 1, 1970 00:00:00 UTC. This method
   * shouldn't be used for measuring timeouts or other elapsed time measurements, as changing the
   * system time can affect the results.
   *
   * @return the local system time in milliseconds.
   */
  long currentTimeMillis();

  /**
   * Returns milliseconds since boot, including time spent in sleep.
   *
   * @return elapsed milliseconds since boot.
   */
  long elapsedRealtime();

  /**
   * Returns nanoseconds since boot, including time spent in sleep.
   *
   * @return elapsed nanoseconds since boot.
   */
  default long elapsedRealtimeNanos() {
    return NS_IN_MS * elapsedRealtime();
  }

  /**
   * Returns milliseconds since boot, not counting time spent in deep sleep.
   *
   * @return milliseconds of non-sleep uptime since boot.
   */
  long uptimeMillis();
}
