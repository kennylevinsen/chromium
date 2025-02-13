// Copyright 2019 The Feed Authors.
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

package com.google.android.libraries.feed.common.intern;

import com.google.protobuf.GeneratedMessageLite;
import com.google.protobuf.MessageLite;
import java.util.ArrayList;
import java.util.List;
import javax.annotation.concurrent.ThreadSafe;

/**
 * A string-specific {@link Interner} base implementation that provides common helper methods to
 * help in proto string interning.
 */
@ThreadSafe
public abstract class ProtoStringInternerBase<P extends MessageLite> implements Interner<P> {

  private final Interner<String> interner;

  protected ProtoStringInternerBase(Interner<String> interner) {
    this.interner = interner;
  }

  protected interface SingleStringFieldGetter<T extends GeneratedMessageLite<T, ?>> {
    String getField(T input);
  }

  protected interface SingleStringFieldSetter<B extends GeneratedMessageLite.Builder<?, B>> {
    void setField(B builder, String value);
  }

  @SuppressWarnings("ReferenceEquality") // Intentional reference comparison for interned != orig
  /*@Nullable*/
  protected <T extends GeneratedMessageLite<T, B>, B extends GeneratedMessageLite.Builder<T, B>>
      B internSingleStringField(
          T input,
          /*@Nullable*/ B builder,
          SingleStringFieldGetter<T> singleStringFieldGetter,
          SingleStringFieldSetter<B> singleStringFieldSetter) {
    String orig = singleStringFieldGetter.getField(input);
    String interned = interner.intern(orig);
    if (interned != orig) {
      builder = ensureBuilder(input, builder);
      singleStringFieldSetter.setField(builder, interned);
    }
    return builder;
  }

  protected interface RepeatedStringFieldGetter<T extends GeneratedMessageLite<T, ?>> {
    List<String> getField(T input);
  }

  protected interface RepeatedStringFieldClearer<B extends GeneratedMessageLite.Builder<?, B>> {
    void clearField(B builder);
  }

  protected interface RepeatedStringFieldAllAdder<B extends GeneratedMessageLite.Builder<?, B>> {
    void addAllField(B builder, List<String> value);
  }

  @SuppressWarnings("ReferenceEquality") // Intentional reference comparison for interned != orig
  /*@Nullable*/
  protected <T extends GeneratedMessageLite<T, B>, B extends GeneratedMessageLite.Builder<T, B>>
      B internRepeatedStringField(
          T input,
          /*@Nullable*/ B builder,
          RepeatedStringFieldGetter<T> repeatedStringFieldGetter,
          RepeatedStringFieldClearer<B> repeatedStringFieldClearer,
          RepeatedStringFieldAllAdder<B> repeatedStringFieldAllAdder) {
    boolean modified = false;
    List<String> internedValues = new ArrayList<>();
    for (String orig : repeatedStringFieldGetter.getField(input)) {
      String interned = interner.intern(orig);
      internedValues.add(interned);
      if (interned != orig) {
        modified = true;
      }
    }
    if (modified) {
      builder = ensureBuilder(input, builder);
      repeatedStringFieldClearer.clearField(builder);
      repeatedStringFieldAllAdder.addAllField(builder, internedValues);
    }
    return builder;
  }

  protected <T extends GeneratedMessageLite<T, B>, B extends GeneratedMessageLite.Builder<T, B>>
      B ensureBuilder(T input, /*@Nullable*/ B builder) {
    if (builder == null) {
      builder = input.toBuilder();
    }
    return builder;
  }

  @Override
  public void clear() {
    interner.clear();
  }

  @Override
  public int size() {
    return interner.size();
  }
}
