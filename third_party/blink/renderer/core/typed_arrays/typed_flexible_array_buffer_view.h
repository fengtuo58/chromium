// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_TYPED_FLEXIBLE_ARRAY_BUFFER_VIEW_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_TYPED_FLEXIBLE_ARRAY_BUFFER_VIEW_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/typed_arrays/flexible_array_buffer_view.h"

namespace blink {

template <typename TypedArray>
class TypedFlexibleArrayBufferView final : public FlexibleArrayBufferView {
  STACK_ALLOCATED();

 public:
  using ValueType = typename TypedArray::ValueType;

  TypedFlexibleArrayBufferView() : FlexibleArrayBufferView() {}

  ValueType* DataMaybeOnStack() const {
    return static_cast<ValueType*>(BaseAddressMaybeOnStack());
  }

  unsigned length() const {
    DCHECK_EQ(ByteLength() % sizeof(ValueType), 0u);
    return ByteLength() / sizeof(ValueType);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TypedFlexibleArrayBufferView);
};

using FlexibleFloat32ArrayView =
    TypedFlexibleArrayBufferView<TypedArray<float>>;
using FlexibleInt32ArrayView =
    TypedFlexibleArrayBufferView<TypedArray<int32_t>>;
using FlexibleUint32ArrayView =
    TypedFlexibleArrayBufferView<TypedArray<uint32_t>>;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_TYPED_FLEXIBLE_ARRAY_BUFFER_VIEW_H_
