#ifndef DYNSCAN_JACCARD_H
#define DYNSCAN_JACCARD_H

#include "Vertex.h"
#include <boost/serialization/access.hpp>
#include <random>
#include <sstream>
#include <string>

class Jaccard {
  friend class boost::serialization::access;

private:
  std::mt19937_64 gen;
  std::uniform_int_distribution<long long> dis;
  unsigned long long size_v;
  unsigned long long size_u;
  unsigned long long index;
  unsigned long long common;
  unsigned long long numSamples;
  double mean;
  double rho;

  long double one_over_failure_prob;
  long double one_over_failure_prob_current_invoke;

  long long number_invoke;
  long long batch_num;
  long long batch_size;

  inline bool is_a_common_vertex(const dynscan::Vertex &_v,
                                 const dynscan::Vertex &_u) {
    if (index <= size_v) {
      const int v_neighbor = index < size_v ? _v.getNeighborID(index) : _v.id;
      if (_u.has_neighbor(v_neighbor)) {
        return true;
      }
    } else {
      index = index - size_v - 1;
      const int u_neighbor = index < size_u ? _u.getNeighborID(index) : _u.id;
      if (_v.has_neighbor(u_neighbor)) {
        return true;
      }
    }
    return false;
  }

  /**
   *
   * @param _var variance
   * @param _current_failure_prob failure probability allocated at current round
   * @param _num_samples
   * @return
   */
  inline double bernstein_interval(const double &_var,
                                   const double &_one_over_current_failure_prob,
                                   const int &_num_samples) const {
    return std::sqrt((_var * log(3 * _one_over_current_failure_prob)) /
                     _num_samples) +
           (3 * log(3 * _one_over_current_failure_prob) / _num_samples);
  }

public:
  static double jaccard_brute_force(const dynscan::Vertex *_v,
                                    const dynscan::Vertex *_u) {
    const dynscan::Vertex &v1 = _v->getDegree() < _u->getDegree() ? *_v : *_u;
    const dynscan::Vertex &v2 = _v->getDegree() < _u->getDegree() ? *_u : *_v;
    const int degree1 = v1.getDegree();
    const int degree2 = v2.getDegree();
    int intersectionCnt = 2;
    for (int i = 0; i < degree1; ++i) {
      if (v2.getAdjacentIndex(v1.getNeighborID(i)) != -1) {
        ++intersectionCnt;
      }
    }
    return intersectionCnt / (double)(degree1 + degree2 + 2 - intersectionCnt);
  }

  /**
   *
   * @param _one_over_failure_prob one over the failure probability
   * @param _alpha_rho alpha times rho defined on paper
   */
  Jaccard(long double _one_over_failure_prob, const double &_alpha_rho)
      : Jaccard(_one_over_failure_prob, _alpha_rho, std::random_device{}()) {}

  /**
   *
   * @param _one_over_failure_prob one over the failure probability
   * @param _alpha_rho alpha times rho defined on paper
   * @param seed The number used to seed the random generator
   */
  Jaccard(long double _one_over_failure_prob, const double &_alpha_rho,
          unsigned long long seed, const long long _batch_size = 500000)
      : gen(seed), rho(_alpha_rho),
        one_over_failure_prob(_one_over_failure_prob), number_invoke(0),
        batch_num(0), batch_size(_batch_size) {
    size_v = 0;
    size_u = 0;
    mean = 0;
    /// oneOverFailure_probability is set to vertex_number, and the power 4 is
    /// extracted out of log
    numSamples = 2.0 / rho / rho * log(one_over_failure_prob);
  }

  Jaccard() = default;

  inline void adjust_failure_prob_current_invoke() {
    ++number_invoke;
    batch_num = number_invoke / batch_size + 1;
    one_over_failure_prob_current_invoke =
        batch_size * batch_num * (batch_num + 1) * one_over_failure_prob;
  }

  /**
   *
   * @param _v
   * @param _u
   * @return true if similar otherwise false
   */
  inline float compute_similarity(const dynscan::Vertex &_v,
                                  const dynscan::Vertex &_u) {

    if (!_v.isLarge()) {
      const int uid = _u.id;
      const int index = _v.getAdjacentIndex(uid);
      // get number of common neighbors between v1 and v2
      const int common = _v.getIntersectionCnt(index);
      const double jaccard =
          common / (double)(_v.getDegree() + _u.getDegree() + 2 - common);
      return jaccard;
    } else if (!_u.isLarge()) {
      const int vid = _v.id;
      const int index = _u.getAdjacentIndex(vid);
      // get number of common neighbors between v1 and v2
      const int common = _u.getIntersectionCnt(index);
      const double jaccard =
          common / (double)(_v.getDegree() + _u.getDegree() + 2 - common);
      return jaccard;
    }

    adjust_failure_prob_current_invoke();
    numSamples = 2.0 / rho / rho * log(one_over_failure_prob_current_invoke);
    size_v = _v.getDegree();
    size_u = _u.getDegree();
    common = 0;
    dis.param(std::uniform_int_distribution<long long>::param_type(
        0, size_v + size_u + 1));
    for (unsigned long long i = 0; i < numSamples; ++i) {
      index = dis(gen);
      if (is_a_common_vertex(_v, _u)) {
        ++common;
      }
    }
    mean = common / (double)numSamples;
    mean = mean / (2 - mean);
    return mean;
  }

private:
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version);
};

template <class Archive>
void Jaccard::serialize(Archive &ar, const unsigned int) {
  ar & size_v;
  ar & size_u;
  ar & index;
  ar & common;
  ar & numSamples;
  ar & mean;
  ar & rho;
  ar & one_over_failure_prob;
  ar & one_over_failure_prob_current_invoke;
  ar & number_invoke;
  ar & batch_num;
  ar & batch_size;

  // std::mt19937_64 is not serialisable field-by-field. its textual stream
  // operators round-trip the whole engine state exactly.
  std::string engine_state;
  if (Archive::is_saving::value) {
    std::ostringstream stream;
    stream << gen;
    engine_state = stream.str();
  }
  ar & engine_state;
  if (Archive::is_loading::value) {
    std::istringstream stream(engine_state);
    stream >> gen;
  }
}

#endif // DYNSCAN_JACCARD_H
