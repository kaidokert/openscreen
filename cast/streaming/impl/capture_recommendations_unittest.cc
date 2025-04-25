// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/public/capture_recommendations.h"

#include <optional>
#include <sstream>

#include "cast/streaming/public/answer_messages.h"
#include "cast/streaming/resolution.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/base/trivial_clock_traits.h"
#include "util/chrono_helpers.h"

namespace openscreen::cast {
namespace capture_recommendations {
namespace {

const Dimensions kMinDimensions{320, 240, 30};
const Dimensions kMaxDimensions{3840, 2160, 60};
constexpr int kMaxEffectiveBitrate = 3840 * 2160 * 60;

const Recommendations kDefaultRecommendations{
    Audio{BitRateLimits{32000, 256000}, milliseconds(400), 2, 48000, 16000},
    Video{BitRateLimits{300000, kMaxEffectiveBitrate}, Resolution{320, 240},
          kMaxDimensions, false, milliseconds(400), kMaxEffectiveBitrate / 8}};

const DisplayDescription kEmptyDescription{};

const DisplayDescription kValidOnlyResolution{
    Dimensions{1024, 768, SimpleFraction{60, 1}}, std::nullopt, std::nullopt};

const DisplayDescription kValidOnlyAspectRatio{std::nullopt, AspectRatio{4, 3},
                                               std::nullopt};

const DisplayDescription kValidOnlyAspectRatioSixteenNine{
    std::nullopt, AspectRatio{16, 9}, std::nullopt};

const DisplayDescription kValidOnlyVariable{std::nullopt, std::nullopt,
                                            AspectRatioConstraint::kVariable};

const DisplayDescription kInvalidOnlyFixed{std::nullopt, std::nullopt,
                                           AspectRatioConstraint::kFixed};

const DisplayDescription kValidFixedAspectRatio{std::nullopt, AspectRatio{4, 3},
                                                AspectRatioConstraint::kFixed};

const DisplayDescription kValidVariableAspectRatio{
    std::nullopt, AspectRatio{4, 3}, AspectRatioConstraint::kVariable};

const DisplayDescription kValidFixedMissingAspectRatio{
    Dimensions{1024, 768, SimpleFraction{60, 1}}, std::nullopt,
    AspectRatioConstraint::kFixed};

const DisplayDescription kValidDisplayFhd{
    Dimensions{1920, 1080, SimpleFraction{30, 1}}, AspectRatio{16, 9},
    AspectRatioConstraint::kVariable};

const DisplayDescription kValidDisplayXga{
    Dimensions{1024, 768, SimpleFraction{60, 1}}, AspectRatio{4, 3},
    AspectRatioConstraint::kFixed};

const DisplayDescription kValidDisplayTiny{
    Dimensions{300, 200, SimpleFraction{30, 1}}, AspectRatio{3, 2},
    AspectRatioConstraint::kFixed};

const DisplayDescription kValidDisplayMismatched{
    Dimensions{300, 200, SimpleFraction{30, 1}}, AspectRatio{3, 4},
    AspectRatioConstraint::kFixed};

const Constraints kEmptyConstraints{};

const Constraints kValidConstraintsHighEnd{
    {96100, 5, 96000, 500000, std::chrono::seconds(6)},
    {6000000, Dimensions{640, 480, SimpleFraction{30, 1}},
     Dimensions{3840, 2160, SimpleFraction{144, 1}}, 600000, 6000000,
     std::chrono::seconds(6)}};

const Constraints kValidConstraintsLowEnd{
    {22000, 2, 24000, 50000, std::chrono::seconds(1)},
    {60000, Dimensions{120, 80, SimpleFraction{10, 1}},
     Dimensions{1200, 800, SimpleFraction{30, 1}}, 100000, 1000000,
     std::chrono::seconds(1)}};

}  // namespace

using clock_operators::operator<<;

std::ostream& operator<<(std::ostream& os, const BitRateLimits& limits) {
  return os << "{minimum: " << limits.minimum << ", maximum: " << limits.maximum
            << "}";
}

std::ostream& operator<<(std::ostream& os, const Audio& audio) {
  return os << "{bit_rate_limits: " << audio.bit_rate_limits
            << ", max_delay: " << audio.max_delay
            << ", max_channels: " << audio.max_channels
            << ", max_sample_rate: " << audio.max_sample_rate
            << ", min_sample_rate: " << audio.min_sample_rate << "}";
}

std::ostream& operator<<(std::ostream& os, const Video& video) {
  return os << "{bit_rate_limits: " << video.bit_rate_limits
            << ", minimum: " << video.minimum << ", maximum: " << video.maximum
            << ", supports_scaling: " << std::boolalpha
            << video.supports_scaling << ", max_delay: " << video.max_delay
            << ", max_pixels_per_second: " << video.max_pixels_per_second
            << "}";
}

std::ostream& operator<<(std::ostream& os, const Recommendations& recs) {
  return os << "{\n  audio: " << recs.audio << ",\n  video: " << recs.video
            << "\n}";
}

MATCHER_P(EqualsRecommendations, expected, "") {
  if (arg == expected) {
    return true;
  }
  *result_listener << "\nExpected: " << expected << "\nActual:   " << arg;
  return false;
}

TEST(CaptureRecommendationsTest, UsesDefaultsIfNoReceiverInformationAvailable) {
  EXPECT_THAT(kDefaultRecommendations, GetRecommendations(Answer{}));
}

TEST(CaptureRecommendationsTest, EmptyDisplayDescription) {
  Answer answer;
  answer.display = kEmptyDescription;
  EXPECT_THAT(GetRecommendations(answer),
              EqualsRecommendations(kDefaultRecommendations));
}

TEST(CaptureRecommendationsTest, OnlyResolution) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.maximum = Dimensions{1024, 768, 60.0};
  expected.video.bit_rate_limits.maximum = 47185920;
  Answer answer;
  answer.display = kValidOnlyResolution;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, OnlyAspectRatioFourThirds) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{320, 240};
  expected.video.maximum = Dimensions{1440, 1080, 30.0};
  Answer answer;
  answer.display = kValidOnlyAspectRatio;

  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, OnlyAspectRatioSixteenNine) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{426, 240};
  expected.video.maximum = Dimensions{1920, 1080, 30.0};
  Answer answer;
  answer.display = kValidOnlyAspectRatioSixteenNine;

  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, OnlyAspectRatioConstraint) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.supports_scaling = true;
  Answer answer;
  answer.display = kValidOnlyVariable;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

// It doesn't make sense to just provide a "fixed" aspect ratio with no
// other dimension information, so we just return default recommendations
// in this case and assume the sender will handle it elsewhere, e.g. on
// ANSWER message parsing.
TEST(CaptureRecommendationsTest, OnlyInvalidAspectRatioConstraint) {
  Answer answer;
  answer.display = kInvalidOnlyFixed;
  EXPECT_THAT(GetRecommendations(answer),
              EqualsRecommendations(kDefaultRecommendations));
}

TEST(CaptureRecommendationsTest, FixedAspectRatioConstraint) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{320, 240};
  expected.video.maximum = Dimensions{1440, 1080, 30.0};
  expected.video.supports_scaling = false;
  Answer answer;
  answer.display = kValidFixedAspectRatio;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

// Our behavior is actually the same whether the constraint is passed, we
// just percolate the constraint up to the capture devices so that intermediate
// frame sizes between minimum and maximum can be properly scaled.
TEST(CaptureRecommendationsTest, VariableAspectRatioConstraint) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{320, 240};
  expected.video.maximum = Dimensions{1440, 1080, 30.0};
  expected.video.supports_scaling = true;
  Answer answer;
  answer.display = kValidVariableAspectRatio;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, ResolutionWithFixedConstraint) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{320, 240};
  expected.video.maximum = Dimensions{1024, 768, 60.0};
  expected.video.supports_scaling = false;
  expected.video.bit_rate_limits.maximum = 47185920;
  Answer answer;
  answer.display = kValidFixedMissingAspectRatio;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, ExplicitFhdChangesMinimum) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{426, 240};
  expected.video.maximum = Dimensions{1920, 1080, 30.0};
  expected.video.bit_rate_limits.maximum = 1920 * 1080 * 30;
  expected.video.supports_scaling = true;
  Answer answer;
  answer.display = kValidDisplayFhd;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, XgaResolution) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{320, 240};
  expected.video.maximum = Dimensions{1024, 768, 60.0};
  expected.video.supports_scaling = false;
  expected.video.bit_rate_limits.maximum = 47185920;
  Answer answer;
  answer.display = kValidDisplayXga;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, MismatchedDisplayAndAspectRatio) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{150, 200};
  expected.video.maximum = Dimensions{150, 200, 30.0};
  expected.video.supports_scaling = false;
  expected.video.bit_rate_limits.maximum = 300 * 200 * 30;
  Answer answer;
  answer.display = kValidDisplayMismatched;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, TinyDisplay) {
  Recommendations expected = kDefaultRecommendations;
  expected.video.minimum = Resolution{300, 200};
  expected.video.maximum = Dimensions{300, 200, 30.0};
  expected.video.supports_scaling = false;
  expected.video.bit_rate_limits.maximum = 300 * 200 * 30;
  Answer answer;
  answer.display = kValidDisplayTiny;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(expected));
}

TEST(CaptureRecommendationsTest, EmptyConstraints) {
  Answer answer;
  answer.constraints = kEmptyConstraints;
  EXPECT_THAT(GetRecommendations(answer),
              EqualsRecommendations(kDefaultRecommendations));
}

// Generally speaking, if the receiver gives us constraints higher than our
// defaults we will accept them, with the exception of maximum resolutions
// exceeding 4K.
TEST(CaptureRecommendationsTest, HandlesHighEnd) {
  const Recommendations kExpected{
      Audio{BitRateLimits{96000, 500000}, milliseconds(6000), 5, 96100, 16000},
      Video{BitRateLimits{600000, 6000000}, Resolution{640, 480},
            kMaxDimensions, false, milliseconds(6000), 6000000}};
  Answer answer;
  answer.constraints = kValidConstraintsHighEnd;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(kExpected));
}

// However, if the receiver gives us constraints lower than our minimum
// defaults, we will ignore them--they would result in an unacceptable cast
// experience.
TEST(CaptureRecommendationsTest, HandlesLowEnd) {
  const Recommendations kExpected{
      Audio{BitRateLimits{32000, 50000}, milliseconds(1000), 2, 22000, 16000},
      Video{BitRateLimits{300000, 1000000}, Resolution{320, 240},
            Dimensions{1200, 800, 30}, false, milliseconds(1000), 60000}};
  Answer answer;
  answer.constraints = kValidConstraintsLowEnd;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(kExpected));
}

TEST(CaptureRecommendationsTest, HandlesTooSmallScreen) {
  const Recommendations kExpected{
      Audio{BitRateLimits{32000, 50000}, milliseconds(1000), 2, 22000, 16000},
      Video{BitRateLimits{300000, 1000000}, Resolution{320, 240},
            kMinDimensions, false, milliseconds(1000), 60000}};
  Answer answer;
  answer.constraints = kValidConstraintsLowEnd;
  answer.constraints->video.max_dimensions =
      answer.constraints->video.min_resolution.value();
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(kExpected));
}

TEST(CaptureRecommendationsTest, HandlesMinimumSizeScreen) {
  const Recommendations kExpected{
      Audio{BitRateLimits{32000, 50000}, milliseconds(1000), 2, 22000, 16000},
      Video{BitRateLimits{300000, 1000000}, Resolution{320, 240},
            kMinDimensions, false, milliseconds(1000), 60000}};
  Answer answer;
  answer.constraints = kValidConstraintsLowEnd;
  answer.constraints->video.max_dimensions =
      Dimensions{320, 240, SimpleFraction{30, 1}};
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(kExpected));
}

TEST(CaptureRecommendationsTest, UsesIntersectionOfDisplayAndConstraints) {
  const Recommendations kExpected{
      Audio{BitRateLimits{96000, 500000}, milliseconds(6000), 5, 96100, 16000},
      Video{BitRateLimits{600000, 6000000}, Resolution{640, 480},
            // Max resolution should be 1080P, since that's the display
            // resolution. No reason to capture at 4K, even though the
            // receiver supports it.
            Dimensions{1920, 1080, 30}, true, milliseconds(6000), 6000000}};
  Answer answer;
  answer.display = kValidDisplayFhd;
  answer.constraints = kValidConstraintsHighEnd;
  EXPECT_THAT(GetRecommendations(answer), EqualsRecommendations(kExpected));
}

}  // namespace capture_recommendations
}  // namespace openscreen::cast
