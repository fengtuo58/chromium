// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/binary_size/libsupersize/caspian/tree_builder.h"

#include <stdint.h>

#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/binary_size/libsupersize/caspian/model.h"

namespace caspian {

namespace {

using FilterList = std::vector<std::function<bool(const BaseSymbol&)>>;

void MakeSymbol(SizeInfo* info,
                SectionId section_id,
                int32_t size,
                const char* path,
                const char* component,
                std::string_view name = "") {
  static std::deque<std::string> symbol_names;
  if (name.empty()) {
    symbol_names.push_back(std::string());
    std::string& s = symbol_names.back();
    s += static_cast<char>(section_id);
    s += "_";
    s += std::to_string(size);
    s += "A";
    name = s;
  }
  Symbol sym;
  sym.full_name_ = name;
  sym.section_id_ = section_id;
  sym.size_ = size;
  sym.source_path_ = path;
  sym.component_ = component;
  sym.size_info_ = info;
  info->raw_symbols.push_back(sym);
}

std::string ShortName(const Json::Value& node) {
  const std::string id_path = node["idPath"].asString();
  const int short_name_index = node["shortNameIndex"].asInt();
  return id_path.substr(short_name_index);
}
}  // namespace

std::unique_ptr<SizeInfo> CreateSizeInfo() {
  std::unique_ptr<SizeInfo> info = std::make_unique<SizeInfo>();
  MakeSymbol(info.get(), SectionId::kText, 20, "a/b/c", "A");
  MakeSymbol(info.get(), SectionId::kText, 30, "a/b", "B");
  return info;
}

void CheckAllTreeNodesFindable(TreeBuilder& tree, const Json::Value& node) {
  const Json::Value& children = node["children"];
  for (unsigned int i = 0; i < children.size(); i++) {
    // Only recurse on folders, which have type Ct or Dt rather than t.
    if (children[i]["type"].size() == 2) {
      std::string id_path = children[i]["idPath"].asString();
      CheckAllTreeNodesFindable(tree, tree.Open(id_path.c_str()));
    }
  }
}

TEST(TreeBuilderTest, TestIdPathLens) {
  std::unique_ptr<SizeInfo> size_info = CreateSizeInfo();

  TreeBuilder builder(size_info.get());
  FilterList filters;
  builder.Build(std::make_unique<IdPathLens>(), '/', false, filters);
  CheckAllTreeNodesFindable(builder, builder.Open(""));
  EXPECT_EQ("Dt", builder.Open("")["type"].asString());
}

TEST(TreeBuilderTest, TestComponentLens) {
  std::unique_ptr<SizeInfo> size_info = CreateSizeInfo();

  TreeBuilder builder(size_info.get());
  FilterList filters;
  builder.Build(std::make_unique<ComponentLens>(), '>', false, filters);
  CheckAllTreeNodesFindable(builder, builder.Open(""));
  EXPECT_EQ("Ct", builder.Open("A")["type"].asString());
  EXPECT_EQ(20, builder.Open("A")["size"].asInt());
  EXPECT_EQ("Ct", builder.Open("B")["type"].asString());
  EXPECT_EQ(30, builder.Open("B")["size"].asInt());
}

TEST(TreeBuilderTest, TestTemplateLens) {
  std::unique_ptr<SizeInfo> size_info = std::make_unique<SizeInfo>();
  MakeSymbol(size_info.get(), SectionId::kText, 20, "a/b/c", "A",
             "base::internal::Invoker<base::internal::BindState<void "
             "(autofill_assistant::Controller::*)(), "
             "base::WeakPtr<autofill_assistant::Controller> >, void "
             "()>::RunOnce(base::internal::BindStateBase*)");
  MakeSymbol(size_info.get(), SectionId::kText, 30, "a/b", "B",
             "base::internal::Invoker<base::internal::BindState<void "
             "(autofill_assistant::Controller::*)(int), "
             "base::WeakPtr<autofill_assistant::Controller>, unsigned int>, "
             "void ()>::RunOnce(base::internal::BindStateBase*)");

  TreeBuilder builder(size_info.get());
  FilterList filters;
  builder.Build(std::make_unique<TemplateLens>(), '/', false, filters);
  CheckAllTreeNodesFindable(builder, builder.Open(""));
  EXPECT_EQ(
      "Ct",
      builder.Open("base::internal::Invoker<>::RunOnce")["type"].asString());
  EXPECT_EQ(50,
            builder.Open("base::internal::Invoker<>::RunOnce")["size"].asInt());
}

TEST(TreeBuilderTest, TestNoNameUnderGroup) {
  std::unique_ptr<SizeInfo> size_info = std::make_unique<SizeInfo>();
  MakeSymbol(size_info.get(), SectionId::kText, 20, "", "A>B>C", "SymbolName");

  TreeBuilder builder(size_info.get());
  FilterList filters;
  builder.Build(std::make_unique<ComponentLens>(), '>', false, filters);
  CheckAllTreeNodesFindable(builder, builder.Open(""));
  EXPECT_EQ("A>B>C/(No path)",
            builder.Open("A>B>C")["children"][0]["idPath"].asString());
}

TEST(TreeBuilderTest, TestJoinDexMethodClasses) {
  std::unique_ptr<SizeInfo> size_info = std::make_unique<SizeInfo>();
  MakeSymbol(size_info.get(), SectionId::kDex, 30,
             "chrome/android/features/start_surface/public/java/src/org/"
             "chromium/chrome/features/start_surface/StartSurface.java",
             "Mobile",
             "org.chromium.chrome.features.start_surface.StartSurface$"
             "OverviewModeObserver android.graphics.Bitmap a()");
  MakeSymbol(size_info.get(), SectionId::kDex, 20,
             "chrome/android/features/start_surface/public/java/src/org/"
             "chromium/chrome/features/start_surface/StartSurface.java",
             "Mobile",
             "org.chromium.chrome.features.start_surface.StartSurface$"
             "OverviewModeObserver <init>(android.graphics.Bitmap)");

  TreeBuilder builder(size_info.get());
  FilterList filters;
  builder.Build(std::make_unique<ComponentLens>(), '>', false, filters);
  CheckAllTreeNodesFindable(builder, builder.Open(""));

  Json::Value class_symbol = (builder.Open(
      "Mobile/chrome/android/features/start_surface/public/java/src/"
      "org/chromium/chrome/features/start_surface/StartSurface.java"))
      ["children"][0];
  EXPECT_EQ("StartSurface$OverviewModeObserver", ShortName(class_symbol));

  EXPECT_EQ(
      "Mobile/chrome/android/features/start_surface/public/java/src/"
      "org/chromium/chrome/features/start_surface/StartSurface.java/"
      "org.chromium.chrome.features.start_surface.StartSurface$"
      "OverviewModeObserver",
      class_symbol["idPath"].asString());
  EXPECT_EQ(2u, class_symbol["children"].size());

  Json::Value dex_symbol = class_symbol["children"][0];
  EXPECT_EQ(0u, dex_symbol["children"].size());

  EXPECT_EQ("android.graphics.Bitmap a()", ShortName(dex_symbol));
}
}  // namespace caspian
