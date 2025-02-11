/*
 * Copyright (c) 2012 Motorola Mobility, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA MOBILITY, INC. AND ITS CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY, INC. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/radio_node_list.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node_rare_data.h"
#include "third_party/blink/renderer/core/html/forms/html_form_element.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/html/html_object_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input_type_names.h"

namespace blink {

RadioNodeList::RadioNodeList(ContainerNode& owner_node,
                             CollectionType type,
                             const AtomicString& name)
    : LiveNodeList(owner_node,
                   type,
                   kInvalidateForFormControls,
                   IsA<HTMLFormElement>(owner_node)
                       ? NodeListSearchRoot::kTreeScope
                       : NodeListSearchRoot::kOwnerNode),
      name_(name) {
  DCHECK(type == kRadioNodeListType || type == kRadioImgNodeListType);
}

RadioNodeList::~RadioNodeList() = default;

static inline HTMLInputElement* ToRadioButtonInputElement(Element& element) {
  if (!IsHTMLInputElement(element))
    return nullptr;
  HTMLInputElement& input_element = ToHTMLInputElement(element);
  if (input_element.type() != input_type_names::kRadio ||
      input_element.value().IsEmpty())
    return nullptr;
  return &input_element;
}

String RadioNodeList::value() const {
  if (ShouldOnlyMatchImgElements())
    return String();
  unsigned length = this->length();
  for (unsigned i = 0; i < length; ++i) {
    const HTMLInputElement* input_element = ToRadioButtonInputElement(*item(i));
    if (!input_element || !input_element->checked())
      continue;
    return input_element->value();
  }
  return String();
}

void RadioNodeList::setValue(const String& value) {
  if (ShouldOnlyMatchImgElements())
    return;
  unsigned length = this->length();
  for (unsigned i = 0; i < length; ++i) {
    HTMLInputElement* input_element = ToRadioButtonInputElement(*item(i));
    if (!input_element || input_element->value() != value)
      continue;
    input_element->setChecked(true);
    return;
  }
}

bool RadioNodeList::MatchesByIdOrName(const Element& test_element) const {
  return test_element.GetIdAttribute() == name_ ||
         test_element.GetNameAttribute() == name_;
}

bool RadioNodeList::CheckElementMatchesRadioNodeListFilter(
    const Element& test_element) const {
  DCHECK(!ShouldOnlyMatchImgElements());
  DCHECK(IsA<HTMLObjectElement>(test_element) ||
         test_element.IsFormControlElement());
  if (IsA<HTMLFormElement>(ownerNode())) {
    auto* form_element = To<HTMLElement>(test_element).formOwner();
    if (!form_element || form_element != ownerNode())
      return false;
  }

  return MatchesByIdOrName(test_element);
}

bool RadioNodeList::ElementMatches(const Element& element) const {
  if (ShouldOnlyMatchImgElements()) {
    auto* html_image_element = DynamicTo<HTMLImageElement>(element);
    if (!html_image_element)
      return false;

    if (html_image_element->formOwner() != ownerNode())
      return false;

    return MatchesByIdOrName(element);
  }

  if (!IsA<HTMLObjectElement>(element) && !element.IsFormControlElement())
    return false;

  if (IsHTMLInputElement(element) &&
      ToHTMLInputElement(element).type() == input_type_names::kImage)
    return false;

  return CheckElementMatchesRadioNodeListFilter(element);
}

}  // namespace blink
