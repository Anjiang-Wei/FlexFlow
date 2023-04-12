#ifndef _FLEXFLOW_UTILS_GRAPH_NODE_H
#define _FLEXFLOW_UTILS_GRAPH_NODE_H

#include <cstddef>
#include <functional>
#include <unordered_set>
#include <ostream>
#include "utils/visitable.h"
#include "utils/optional.h"
#include "utils/fmt.h"
#include <memory>
#include "utils/type_traits.h"
#include "utils/strong_typedef.h"

namespace FlexFlow {

struct Node : public strong_typedef<Node, size_t> { 
  using strong_typedef::strong_typedef;
};

std::ostream &operator<<(std::ostream &, Node const &);

}

MAKE_TYPEDEF_HASHABLE(::FlexFlow::Node);
MAKE_TYPEDEF_PRINTABLE(::FlexFlow::Node, "Node");

namespace FlexFlow {

struct NodeQuery {
  NodeQuery() = default;
  NodeQuery(std::unordered_set<Node> const &nodes);
  NodeQuery(tl::optional<std::unordered_set<Node>> const &nodes);

  tl::optional<std::unordered_set<Node>> nodes = tl::nullopt;
};

NodeQuery query_intersection(NodeQuery const &, NodeQuery const &);
NodeQuery query_union(NodeQuery const &, NodeQuery const &);

struct IGraphView {
  IGraphView() = default;
  IGraphView(IGraphView const &) = delete;
  IGraphView &operator=(IGraphView const &) = delete;

  virtual std::unordered_set<Node> query_nodes(NodeQuery const &) const = 0;
  virtual ~IGraphView() {};
};

static_assert(is_rc_copy_virtual_compliant<IGraphView>::value, RC_COPY_VIRTUAL_MSG);

struct IGraph : IGraphView {
  IGraph(IGraph const &) = delete;
  IGraph &operator=(IGraph const &) = delete;

  virtual Node add_node() = 0;
  virtual void add_node_unsafe(Node const &) = 0;
  virtual void remove_node_unsafe(Node const &) = 0;
  virtual IGraph *clone() const = 0;
};

static_assert(is_rc_copy_virtual_compliant<IGraph>::value, RC_COPY_VIRTUAL_MSG);

}

#endif 
