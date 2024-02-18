#ifndef _FLEXFLOW_LIB_UTILS_BIDICT_INCLUDE_UTILS_BIDICT_BIDICT_H
#define _FLEXFLOW_LIB_UTILS_BIDICT_INCLUDE_UTILS_BIDICT_BIDICT_H

#include <cassert>
#include <unordered_map>

namespace FlexFlow {

template <typename L, typename R>
struct bidict {
  bidict() : fwd_map{}, bwd_map{} {}

  template <typename InputIt>
  bidict(InputIt first, InputIt last) {
    for (auto it = first; it != last; it++) {
      this->equate(it->first, it->second);
    }
  }

  bidict(std::unordered_map<L, R> const &fwd_map,
         std::unordered_map<R, L> const &bwd_map)
      : fwd_map(fwd_map), bwd_map(bwd_map) {}

  void erase_l(L const &l) {
    this->fwd_map.erase(l);
    for (auto const &kv : this->bwd_map) {
      if (kv.second == l) {
        bwd_map.erase(kv.first);
        break;
      }
    }
  }

  void erase_r(R const &r) {
    this->bwd_map.erase(r);
    for (auto const &kv : this->fwd_map) {
      if (kv.second == r) {
        fwd_map.erase(kv.first);
        break;
      }
    }
  }

  void equate(L const &l, R const &r) {
    fwd_map.insert({l, r});
    bwd_map.insert({r, l});
  }

  void equate(std::pair<L, R> const &lr) {
    fwd_map.insert(lr);
    bwd_map.insert({lr.second, lr.first});
  }

  R const &at_l(L const &l) const {
    return fwd_map.at(l);
  }
  L const &at_r(R const &r) const {
    return bwd_map.at(r);
  }

  std::size_t size() const {
    this->check_invariants();
    return fwd_map.size();
  }

  bool empty() const {
    this->check_invariants();
    return fwd_map.empty();
  }

  using const_iterator = typename std::unordered_map<L, R>::const_iterator;
  using value_type = std::pair<L, R>;
  using reference = value_type &;
  using const_reference = value_type const &;
  using key_type = L;
  using mapped_type = R;

  const_iterator cbegin() const {
    return this->fwd_map.cbegin();
  }

  const_iterator begin() const {
    return this->cbegin();
  }

  const_iterator cend() const {
    return this->fwd_map.cend();
  }

  const_iterator end() const {
    return this->cend();
  }

  const_iterator find_l(L const &l) const {
    return this->fwd_map.find(l);
  }

  const_iterator find_r(R const &r) const {
    auto it = this->bwd_map.find(r);
    if (it == this->bwd_map.end()) {
      return this->fwd_map.end();
    } else {
      return this->fwd_map.find(it->second);
    }
  }

  bidict<R, L> reversed() const {
    return bidict<R, L>(bwd_map, fwd_map);
  }

  operator std::unordered_map<L, R> const &() const {
    return this->fwd_map;
  }

  friend bool operator==(bidict<L, R> const &lhs, 
                         bidict<L, R> const &rhs) {
    lhs.check_invariants();
    rhs.check_invariants();
    return lhs.fwd_map == rhs.fwd_map;
  }

  friend bool operator!=(bidict<L, R> const &lhs,
                         bidict<L, R> const &rhs) {
    lhs.check_invariants();
    rhs.check_invariants();
    return lhs.fwd_map != rhs.fwd_map;
  }
private:
  void check_invariants() const {
    assert(fwd_map.size() == bwd_map.size());
  }

  friend struct bidict<R, L>;

  std::unordered_map<L, R> fwd_map;
  std::unordered_map<R, L> bwd_map;
};

} // namespace FlexFlow

#endif

