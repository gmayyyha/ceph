// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#pragma once

#include <string>
#include <vector>

//#include <boost/iterator/counting_iterator.hpp>

#include "crimson/common/log.h"
#include "crimson/os/seastore/seastore_types.h"
#include "crimson/os/seastore/transaction_manager.h"
#include "crimson/os/seastore/omap_manager.h"
#include "crimson/os/seastore/omap_manager/btree/omap_types.h"

namespace crimson::os::seastore::omap_manager{

struct omap_context_t {
  InterruptedTransactionManager tm;
  Transaction &t;
};

enum class mutation_status_t : uint8_t {
  SUCCESS = 0,
  WAS_SPLIT = 1,
  NEED_MERGE = 2,
  FAIL = 3
};

struct OMapNode : LogicalCachedExtent {
  using base_ertr = OMapManager::base_ertr;

  using OMapNodeRef = TCachedExtentRef<OMapNode>;

  struct mutation_result_t {
    mutation_status_t status;
    /// Only populated if WAS_SPLIT, indicates the newly created left and right nodes
    /// from splitting the target entry during insertion.
    std::optional<std::tuple<OMapNodeRef, OMapNodeRef, std::string>> split_tuple;
    /// only sopulated if need merged, indicate which entry need be doing merge in upper layer.
    std::optional<OMapNodeRef> need_merge;

    mutation_result_t(mutation_status_t s, std::optional<std::tuple<OMapNodeRef,
                      OMapNodeRef, std::string>> tuple, std::optional<OMapNodeRef> n_merge)
    : status(s),
      split_tuple(tuple),
      need_merge(n_merge) {}
  };

  OMapNode(ceph::bufferptr &&ptr) : LogicalCachedExtent(std::move(ptr)) {}
  OMapNode(const OMapNode &other)
  : LogicalCachedExtent(other) {}

  using get_value_ertr = base_ertr;
  using get_value_ret = OMapManager::omap_get_value_ret;
  virtual get_value_ret get_value(
    omap_context_t oc,
    const std::string &key) = 0;

  using insert_ertr = base_ertr;
  using insert_ret = insert_ertr::future<mutation_result_t>;
  virtual insert_ret insert(
    omap_context_t oc,
    const std::string &key,
    const ceph::bufferlist &value) = 0;

  using rm_key_ertr = base_ertr;
  using rm_key_ret = rm_key_ertr::future<mutation_result_t>;
  virtual rm_key_ret rm_key(
    omap_context_t oc,
    const std::string &key) = 0;

  using omap_list_config_t = OMapManager::omap_list_config_t;
  using list_ertr = base_ertr;
  using list_bare_ret = OMapManager::omap_list_bare_ret;
  using list_ret = OMapManager::omap_list_ret;
  virtual list_ret list(
    omap_context_t oc,
    const std::optional<std::string> &start,
    omap_list_config_t config) = 0;

  using clear_ertr = base_ertr;
  using clear_ret = clear_ertr::future<>;
  virtual clear_ret clear(omap_context_t oc) = 0;

  using full_merge_ertr = base_ertr;
  using full_merge_ret = full_merge_ertr::future<OMapNodeRef>;
  virtual full_merge_ret make_full_merge(
    omap_context_t oc,
    OMapNodeRef right) = 0;

  using make_balanced_ertr = base_ertr;
  using make_balanced_ret = make_balanced_ertr::future
          <std::tuple<OMapNodeRef, OMapNodeRef, std::string>>;
  virtual make_balanced_ret make_balanced(
    omap_context_t oc,
    OMapNodeRef _right) = 0;

  virtual omap_node_meta_t get_node_meta() const = 0;
  virtual bool extent_will_overflow(
    size_t ksize,
    std::optional<size_t> vsize) const = 0;
  virtual bool extent_is_below_min() const = 0;
  virtual uint32_t get_node_size() = 0;

  virtual ~OMapNode() = default;
};

using OMapNodeRef = OMapNode::OMapNodeRef;

using omap_load_extent_ertr = OMapNode::base_ertr;
omap_load_extent_ertr::future<OMapNodeRef>
omap_load_extent(omap_context_t oc, laddr_t laddr, depth_t depth);

}
