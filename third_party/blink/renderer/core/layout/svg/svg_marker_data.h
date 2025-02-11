/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_SVG_SVG_MARKER_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_SVG_SVG_MARKER_DATA_H_

#include "third_party/blink/renderer/platform/graphics/path.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

enum SVGMarkerType { kStartMarker, kMidMarker, kEndMarker };

class LayoutSVGResourceMarker;

struct MarkerPosition {
  DISALLOW_NEW();
  MarkerPosition(SVGMarkerType use_type,
                 const FloatPoint& use_origin,
                 float use_angle)
      : type(use_type), origin(use_origin), angle(use_angle) {}

  LayoutSVGResourceMarker* SelectMarker(
      LayoutSVGResourceMarker* marker_start,
      LayoutSVGResourceMarker* marker_mid,
      LayoutSVGResourceMarker* marker_end) const {
    switch (type) {
      case kStartMarker:
        return marker_start;
      case kMidMarker:
        return marker_mid;
      case kEndMarker:
        return marker_end;
    }
    NOTREACHED();
    return nullptr;
  }

  SVGMarkerType type;
  FloatPoint origin;
  float angle;
};

class SVGMarkerDataBuilder {
  STACK_ALLOCATED();

 public:
  SVGMarkerDataBuilder(Vector<MarkerPosition>& positions)
      : positions_(positions), element_index_(0) {}

  void Build(const Path&);

 private:
  static void UpdateFromPathElement(void* info, const PathElement*);
  void PathIsDone();

  double CurrentAngle(SVGMarkerType type) const;

  struct SegmentData {
    FloatSize start_tangent;  // Tangent in the start point of the segment.
    FloatSize end_tangent;    // Tangent in the end point of the segment.
    FloatPoint position;      // The end point of the segment.
  };

  static void ComputeQuadTangents(SegmentData&,
                                  const FloatPoint& start,
                                  const FloatPoint& control,
                                  const FloatPoint& end);
  SegmentData ExtractPathElementFeatures(const PathElement&) const;
  void UpdateFromPathElement(const PathElement&);

  Vector<MarkerPosition>& positions_;
  unsigned element_index_;
  FloatPoint origin_;
  FloatPoint subpath_start_;
  FloatSize in_slope_;
  FloatSize out_slope_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_SVG_SVG_MARKER_DATA_H_
