// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spellcheck/common/spellcheck_features.h"

#include "base/system/sys_info.h"
#include "base/win/windows_version.h"
#include "build/build_config.h"
#include "components/spellcheck/spellcheck_buildflags.h"

namespace spellcheck {

#if BUILDFLAG(ENABLE_SPELLCHECK)

#if defined(OS_WIN)
const base::Feature kWinUseBrowserSpellChecker{
    "WinUseBrowserSpellChecker", base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_WIN)

bool UseBrowserSpellChecker() {
#if !BUILDFLAG(USE_BROWSER_SPELLCHECKER)
  return false;
#elif defined(OS_WIN)
  return base::FeatureList::IsEnabled(spellcheck::kWinUseBrowserSpellChecker) &&
         WindowsVersionSupportsSpellchecker();
#else
  return true;
#endif
}

#if defined(OS_WIN)
bool WindowsVersionSupportsSpellchecker() {
  return base::win::GetVersion() > base::win::Version::WIN7 &&
         base::win::GetVersion() < base::win::Version::WIN_LAST;
}
#endif  // defined(OS_WIN)

#endif  // BUILDFLAG(ENABLE_SPELLCHECK)

#if BUILDFLAG(ENABLE_SPELLCHECK) && defined(OS_ANDROID)

// Enables/disables Android spellchecker.
const base::Feature kAndroidSpellChecker{
    "AndroidSpellChecker", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables/disables Android spellchecker on non low-end Android devices.
const base::Feature kAndroidSpellCheckerNonLowEnd{
    "AndroidSpellCheckerNonLowEnd", base::FEATURE_ENABLED_BY_DEFAULT};

bool IsAndroidSpellCheckFeatureEnabled() {
  if (base::FeatureList::IsEnabled(spellcheck::kAndroidSpellCheckerNonLowEnd)) {
    return !base::SysInfo::IsLowEndDevice();
  }

  if (base::FeatureList::IsEnabled(spellcheck::kAndroidSpellChecker)) {
    return true;
  }

  return false;
}

#endif  // BUILDFLAG(ENABLE_SPELLCHECK) && defined(OS_ANDROID)

}  // namespace spellcheck
