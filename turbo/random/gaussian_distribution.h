// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ABSL_RANDOM_GAUSSIAN_DISTRIBUTION_H_
#define ABSL_RANDOM_GAUSSIAN_DISTRIBUTION_H_

// turbo::gaussian_distribution implements the Ziggurat algorithm
// for generating random gaussian numbers.
//
// Implementation based on "The Ziggurat Method for Generating Random Variables"
// by George Marsaglia and Wai Wan Tsang: http://www.jstatsoft.org/v05/i08/
//

#include <cmath>
#include <cstdint>
#include <istream>
#include <limits>
#include <type_traits>

#include "turbo/base/config.h"
#include "turbo/random/internal/fast_uniform_bits.h"
#include "turbo/random/internal/generate_real.h"
#include "turbo/random/internal/iostream_state_saver.h"

namespace turbo {
ABSL_NAMESPACE_BEGIN
namespace random_internal {

// turbo::gaussian_distribution_base implements the underlying ziggurat algorithm
// using the ziggurat tables generated by the gaussian_distribution_gentables
// binary.
//
// The specific algorithm has some of the improvements suggested by the
// 2005 paper, "An Improved Ziggurat Method to Generate Normal Random Samples",
// Jurgen A Doornik.  (https://www.doornik.com/research/ziggurat.pdf)
class ABSL_DLL gaussian_distribution_base {
 public:
  template <typename URBG>
  inline double zignor(URBG& g);  // NOLINT(runtime/references)

 private:
  friend class TableGenerator;

  template <typename URBG>
  inline double zignor_fallback(URBG& g,  // NOLINT(runtime/references)
                                bool neg);

  // Constants used for the gaussian distribution.
  static constexpr double kR = 3.442619855899;  // Start of the tail.
  static constexpr double kRInv = 0.29047645161474317;  // ~= (1.0 / kR) .
  static constexpr double kV = 9.91256303526217e-3;
  static constexpr uint64_t kMask = 0x07f;

  // The ziggurat tables store the pdf(f) and inverse-pdf(x) for equal-area
  // points on one-half of the normal distribution, where the pdf function,
  // pdf = e ^ (-1/2 *x^2), assumes that the mean = 0 & stddev = 1.
  //
  // These tables are just over 2kb in size; larger tables might improve the
  // distributions, but also lead to more cache pollution.
  //
  // x = {3.71308, 3.44261, 3.22308, ..., 0}
  // f = {0.00101, 0.00266, 0.00554, ..., 1}
  struct Tables {
    double x[kMask + 2];
    double f[kMask + 2];
  };
  static const Tables zg_;
  random_internal::FastUniformBits<uint64_t> fast_u64_;
};

}  // namespace random_internal

// turbo::gaussian_distribution:
// Generates a number conforming to a Gaussian distribution.
template <typename RealType = double>
class gaussian_distribution : random_internal::gaussian_distribution_base {
 public:
  using result_type = RealType;

  class param_type {
   public:
    using distribution_type = gaussian_distribution;

    explicit param_type(result_type mean = 0, result_type stddev = 1)
        : mean_(mean), stddev_(stddev) {}

    // Returns the mean distribution parameter.  The mean specifies the location
    // of the peak.  The default value is 0.0.
    result_type mean() const { return mean_; }

    // Returns the deviation distribution parameter.  The default value is 1.0.
    result_type stddev() const { return stddev_; }

    friend bool operator==(const param_type& a, const param_type& b) {
      return a.mean_ == b.mean_ && a.stddev_ == b.stddev_;
    }

    friend bool operator!=(const param_type& a, const param_type& b) {
      return !(a == b);
    }

   private:
    result_type mean_;
    result_type stddev_;

    static_assert(
        std::is_floating_point<RealType>::value,
        "Class-template turbo::gaussian_distribution<> must be parameterized "
        "using a floating-point type.");
  };

  gaussian_distribution() : gaussian_distribution(0) {}

  explicit gaussian_distribution(result_type mean, result_type stddev = 1)
      : param_(mean, stddev) {}

  explicit gaussian_distribution(const param_type& p) : param_(p) {}

  void reset() {}

  // Generating functions
  template <typename URBG>
  result_type operator()(URBG& g) {  // NOLINT(runtime/references)
    return (*this)(g, param_);
  }

  template <typename URBG>
  result_type operator()(URBG& g,  // NOLINT(runtime/references)
                         const param_type& p);

  param_type param() const { return param_; }
  void param(const param_type& p) { param_ = p; }

  result_type(min)() const {
    return -std::numeric_limits<result_type>::infinity();
  }
  result_type(max)() const {
    return std::numeric_limits<result_type>::infinity();
  }

  result_type mean() const { return param_.mean(); }
  result_type stddev() const { return param_.stddev(); }

  friend bool operator==(const gaussian_distribution& a,
                         const gaussian_distribution& b) {
    return a.param_ == b.param_;
  }
  friend bool operator!=(const gaussian_distribution& a,
                         const gaussian_distribution& b) {
    return a.param_ != b.param_;
  }

 private:
  param_type param_;
};

// --------------------------------------------------------------------------
// Implementation details only below
// --------------------------------------------------------------------------

template <typename RealType>
template <typename URBG>
typename gaussian_distribution<RealType>::result_type
gaussian_distribution<RealType>::operator()(
    URBG& g,  // NOLINT(runtime/references)
    const param_type& p) {
  return p.mean() + p.stddev() * static_cast<result_type>(zignor(g));
}

template <typename CharT, typename Traits, typename RealType>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,  // NOLINT(runtime/references)
    const gaussian_distribution<RealType>& x) {
  auto saver = random_internal::make_ostream_state_saver(os);
  os.precision(random_internal::stream_precision_helper<RealType>::kPrecision);
  os << x.mean() << os.fill() << x.stddev();
  return os;
}

template <typename CharT, typename Traits, typename RealType>
std::basic_istream<CharT, Traits>& operator>>(
    std::basic_istream<CharT, Traits>& is,  // NOLINT(runtime/references)
    gaussian_distribution<RealType>& x) {   // NOLINT(runtime/references)
  using result_type = typename gaussian_distribution<RealType>::result_type;
  using param_type = typename gaussian_distribution<RealType>::param_type;

  auto saver = random_internal::make_istream_state_saver(is);
  auto mean = random_internal::read_floating_point<result_type>(is);
  if (is.fail()) return is;
  auto stddev = random_internal::read_floating_point<result_type>(is);
  if (!is.fail()) {
    x.param(param_type(mean, stddev));
  }
  return is;
}

namespace random_internal {

template <typename URBG>
inline double gaussian_distribution_base::zignor_fallback(URBG& g, bool neg) {
  using random_internal::GeneratePositiveTag;
  using random_internal::GenerateRealFromBits;

  // This fallback path happens approximately 0.05% of the time.
  double x, y;
  do {
    // kRInv = 1/r, U(0, 1)
    x = kRInv *
        std::log(GenerateRealFromBits<double, GeneratePositiveTag, false>(
            fast_u64_(g)));
    y = -std::log(
        GenerateRealFromBits<double, GeneratePositiveTag, false>(fast_u64_(g)));
  } while ((y + y) < (x * x));
  return neg ? (x - kR) : (kR - x);
}

template <typename URBG>
inline double gaussian_distribution_base::zignor(
    URBG& g) {  // NOLINT(runtime/references)
  using random_internal::GeneratePositiveTag;
  using random_internal::GenerateRealFromBits;
  using random_internal::GenerateSignedTag;

  while (true) {
    // We use a single uint64_t to generate both a double and a strip.
    // These bits are unused when the generated double is > 1/2^5.
    // This may introduce some bias from the duplicated low bits of small
    // values (those smaller than 1/2^5, which all end up on the left tail).
    uint64_t bits = fast_u64_(g);
    int i = static_cast<int>(bits & kMask);  // pick a random strip
    double j = GenerateRealFromBits<double, GenerateSignedTag, false>(
        bits);  // U(-1, 1)
    const double x = j * zg_.x[i];

    // Retangular box. Handles >97% of all cases.
    // For any given box, this handles between 75% and 99% of values.
    // Equivalent to U(01) < (x[i+1] / x[i]), and when i == 0, ~93.5%
    if (std::abs(x) < zg_.x[i + 1]) {
      return x;
    }

    // i == 0: Base box. Sample using a ratio of uniforms.
    if (i == 0) {
      // This path happens about 0.05% of the time.
      return zignor_fallback(g, j < 0);
    }

    // i > 0: Wedge samples using precomputed values.
    double v = GenerateRealFromBits<double, GeneratePositiveTag, false>(
        fast_u64_(g));  // U(0, 1)
    if ((zg_.f[i + 1] + v * (zg_.f[i] - zg_.f[i + 1])) <
        std::exp(-0.5 * x * x)) {
      return x;
    }

    // The wedge was missed; reject the value and try again.
  }
}

}  // namespace random_internal
ABSL_NAMESPACE_END
}  // namespace turbo

#endif  // ABSL_RANDOM_GAUSSIAN_DISTRIBUTION_H_
