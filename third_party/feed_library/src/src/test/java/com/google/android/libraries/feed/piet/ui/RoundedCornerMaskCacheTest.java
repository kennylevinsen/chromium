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

package com.google.android.libraries.feed.piet.ui;

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;

import android.graphics.Bitmap;
import com.google.android.libraries.feed.piet.ui.RoundedCornerMaskCache.Corner;
import com.google.android.libraries.feed.piet.ui.RoundedCornerMaskCache.RoundedCornerBitmaps;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests for the {@link RoundedCornerMaskCache}. */
@RunWith(RobolectricTestRunner.class)
public class RoundedCornerMaskCacheTest {

  private RoundedCornerMaskCache cache;

  @Before
  public void setUp() {
    cache = new RoundedCornerMaskCache();
  }

  @Test
  public void testGetBitmaps_createsNewInstance() {
    RoundedCornerBitmaps masks = cache.getMasks(16);

    for (int i = 0; i < 4; i++) {
      Bitmap mask = masks.get(i);
      assertThat(mask.getWidth()).isEqualTo(16);
      assertThat(mask.getHeight()).isEqualTo(16);
    }
  }

  @Test
  public void testGetBitmaps_differentRadii() {
    RoundedCornerBitmaps masksFive = cache.getMasks(5);
    RoundedCornerBitmaps masksTen = cache.getMasks(10);

    assertThat(masksFive).isNotEqualTo(masksTen);

    Bitmap maskFive = masksFive.get(Corner.TOP_LEFT);
    Bitmap maskTen = masksTen.get(Corner.TOP_LEFT);

    assertThat(maskFive).isNotEqualTo(maskTen);
    assertThat(maskFive.getWidth()).isEqualTo(5);
    assertThat(maskTen.getWidth()).isEqualTo(10);
  }

  @Test
  public void testGetBitmaps_cachesInstance() {
    RoundedCornerBitmaps masks1 = cache.getMasks(16);

    RoundedCornerBitmaps masks2 = cache.getMasks(16);

    assertThat(masks1).isSameInstanceAs(masks2);
  }

  @Test
  public void testPurge() {
    RoundedCornerBitmaps masks1 = cache.getMasks(16);

    cache.purge();

    RoundedCornerBitmaps masks2 = cache.getMasks(16);

    assertThat(masks1).isNotSameInstanceAs(masks2);
  }

  @Test
  public void testBadCornerException() {
    RoundedCornerBitmaps masks = cache.getMasks(16);

    assertThatRunnable(() -> masks.get(999))
        .throwsAnExceptionOfType(IllegalArgumentException.class);
  }

  @Test
  public void testCreatesOnlyRequestedCorners() {
    RoundedCornerBitmaps masks = cache.getMasks(123);

    assertThat(masks.masks[Corner.TOP_LEFT]).isNull();
    assertThat(masks.masks[Corner.TOP_RIGHT]).isNull();
    assertThat(masks.masks[Corner.BOTTOM_LEFT]).isNull();
    assertThat(masks.masks[Corner.BOTTOM_RIGHT]).isNull();

    masks.get(Corner.TOP_RIGHT);

    assertThat(masks.masks[Corner.TOP_LEFT]).isNull();
    assertThat(masks.masks[Corner.TOP_RIGHT]).isNotNull();
    assertThat(masks.masks[Corner.BOTTOM_LEFT]).isNull();
    assertThat(masks.masks[Corner.BOTTOM_RIGHT]).isNull();
  }
}
