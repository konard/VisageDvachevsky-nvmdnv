#include <catch2/catch_test_macros.hpp>

// Note: This test file tests the cycle detection logic independently
// Full Qt-based tests for the UI components would require Qt Test framework

// Standalone cycle detection algorithm test
#include <QHash>
#include <QList>
#include <QSet>
#include <functional>
#include <algorithm>

namespace {

// Standalone implementation of cycle detection for testing
// (mirrors the logic from NMStoryGraphScene::wouldCreateCycle)
bool wouldCreateCycle(uint64_t fromNodeId, uint64_t toNodeId,
                      const QHash<uint64_t, QList<uint64_t>> &adjacencyList) {
  if (fromNodeId == toNodeId) {
    return true; // Self-loop
  }

  // Build adjacency list with the proposed edge
  QHash<uint64_t, QList<uint64_t>> adj = adjacencyList;
  adj[fromNodeId].append(toNodeId);

  // DFS from 'to' to see if we can reach 'from'
  QSet<uint64_t> visited;
  QList<uint64_t> stack;
  stack.append(toNodeId);

  while (!stack.isEmpty()) {
    uint64_t current = stack.takeLast();
    if (current == fromNodeId) {
      return true; // Found a cycle
    }
    if (visited.contains(current)) {
      continue;
    }
    visited.insert(current);

    for (uint64_t next : adj.value(current)) {
      if (!visited.contains(next)) {
        stack.append(next);
      }
    }
  }

  return false;
}

// Standalone implementation of Tarjan's algorithm for cycle detection
QList<QList<uint64_t>> detectCycles(const QSet<uint64_t> &allNodes,
                                    const QHash<uint64_t, QList<uint64_t>> &adjacencyList) {
  QList<QList<uint64_t>> cycles;

  // Tarjan's algorithm for strongly connected components
  QHash<uint64_t, int> index;
  QHash<uint64_t, int> lowlink;
  QSet<uint64_t> onStack;
  QList<uint64_t> stack;
  int nextIndex = 0;

  std::function<void(uint64_t)> strongconnect = [&](uint64_t v) {
    index[v] = nextIndex;
    lowlink[v] = nextIndex;
    nextIndex++;
    stack.append(v);
    onStack.insert(v);

    for (uint64_t w : adjacencyList.value(v)) {
      if (!index.contains(w)) {
        strongconnect(w);
        lowlink[v] = qMin(lowlink[v], lowlink[w]);
      } else if (onStack.contains(w)) {
        lowlink[v] = qMin(lowlink[v], index[w]);
      }
    }

    // If v is a root node, pop the stack and generate an SCC
    if (lowlink[v] == index[v]) {
      QList<uint64_t> component;
      uint64_t w;
      do {
        w = stack.takeLast();
        onStack.remove(w);
        component.append(w);
      } while (w != v);

      // Only report SCCs with more than one node (actual cycles)
      if (component.size() > 1) {
        cycles.append(component);
      }
    }
  };

  for (uint64_t nodeId : allNodes) {
    if (!index.contains(nodeId)) {
      strongconnect(nodeId);
    }
  }

  return cycles;
}

} // anonymous namespace

TEST_CASE("Story Graph - Self loop detection", "[story_graph][cycle]") {
  QHash<uint64_t, QList<uint64_t>> adj;

  SECTION("Self-loop is detected") {
    bool hasCycle = wouldCreateCycle(1, 1, adj);
    CHECK(hasCycle == true);
  }
}

TEST_CASE("Story Graph - Simple cycle detection", "[story_graph][cycle]") {
  QHash<uint64_t, QList<uint64_t>> adj;

  SECTION("No cycle in linear graph") {
    // 1 -> 2 -> 3
    adj[1] = {2};
    adj[2] = {3};

    bool hasCycle = wouldCreateCycle(1, 3, adj);
    CHECK(hasCycle == false);
  }

  SECTION("Cycle detected in triangle") {
    // 1 -> 2 -> 3, trying to add 3 -> 1
    adj[1] = {2};
    adj[2] = {3};

    bool hasCycle = wouldCreateCycle(3, 1, adj);
    CHECK(hasCycle == true);
  }

  SECTION("Cycle detected in simple loop") {
    // 1 -> 2, trying to add 2 -> 1
    adj[1] = {2};

    bool hasCycle = wouldCreateCycle(2, 1, adj);
    CHECK(hasCycle == true);
  }
}

TEST_CASE("Story Graph - Complex cycle detection", "[story_graph][cycle]") {
  QHash<uint64_t, QList<uint64_t>> adj;

  SECTION("No cycle in DAG") {
    // Diamond pattern: 1 -> 2, 1 -> 3, 2 -> 4, 3 -> 4
    adj[1] = {2, 3};
    adj[2] = {4};
    adj[3] = {4};

    bool hasCycle = wouldCreateCycle(2, 3, adj);
    CHECK(hasCycle == false);
  }

  SECTION("Cycle detected in complex graph") {
    // 1 -> 2 -> 3 -> 4, trying to add 4 -> 2
    adj[1] = {2};
    adj[2] = {3};
    adj[3] = {4};

    bool hasCycle = wouldCreateCycle(4, 2, adj);
    CHECK(hasCycle == true);
  }

  SECTION("Cycle in disconnected components") {
    // Component 1: 1 -> 2 -> 3
    // Component 2: 4 -> 5, trying to add 5 -> 4
    adj[1] = {2};
    adj[2] = {3};
    adj[4] = {5};

    bool hasCycle = wouldCreateCycle(5, 4, adj);
    CHECK(hasCycle == true);
  }
}

TEST_CASE("Story Graph - Tarjan's algorithm cycle detection",
          "[story_graph][cycle]") {
  SECTION("No cycles in DAG") {
    QSet<uint64_t> nodes = {1, 2, 3, 4};
    QHash<uint64_t, QList<uint64_t>> adj;
    adj[1] = {2, 3};
    adj[2] = {4};
    adj[3] = {4};

    auto cycles = detectCycles(nodes, adj);
    CHECK(cycles.isEmpty());
  }

  SECTION("Single cycle detected") {
    QSet<uint64_t> nodes = {1, 2, 3};
    QHash<uint64_t, QList<uint64_t>> adj;
    adj[1] = {2};
    adj[2] = {3};
    adj[3] = {1}; // Cycle: 1 -> 2 -> 3 -> 1

    auto cycles = detectCycles(nodes, adj);
    CHECK(cycles.size() == 1);
    CHECK(cycles[0].size() == 3);
    // All nodes should be in the cycle
    CHECK(cycles[0].contains(1));
    CHECK(cycles[0].contains(2));
    CHECK(cycles[0].contains(3));
  }

  SECTION("Multiple cycles detected") {
    QSet<uint64_t> nodes = {1, 2, 3, 4, 5, 6};
    QHash<uint64_t, QList<uint64_t>> adj;
    // Cycle 1: 1 -> 2 -> 1
    adj[1] = {2};
    adj[2] = {1};
    // Cycle 2: 4 -> 5 -> 6 -> 4
    adj[4] = {5};
    adj[5] = {6};
    adj[6] = {4};
    // Node 3 is disconnected

    auto cycles = detectCycles(nodes, adj);
    CHECK(cycles.size() == 2);
  }

  SECTION("Nested strongly connected component") {
    QSet<uint64_t> nodes = {1, 2, 3, 4};
    QHash<uint64_t, QList<uint64_t>> adj;
    // All nodes form one big SCC: 1 -> 2 -> 3 -> 4 -> 1
    adj[1] = {2};
    adj[2] = {3};
    adj[3] = {4};
    adj[4] = {1};

    auto cycles = detectCycles(nodes, adj);
    CHECK(cycles.size() == 1);
    CHECK(cycles[0].size() == 4);
  }
}

TEST_CASE("Story Graph - Empty graph", "[story_graph][cycle]") {
  QSet<uint64_t> nodes;
  QHash<uint64_t, QList<uint64_t>> adj;

  SECTION("Empty graph has no cycles") {
    auto cycles = detectCycles(nodes, adj);
    CHECK(cycles.isEmpty());
  }

  SECTION("Adding edge to empty graph creates no cycle") {
    bool hasCycle = wouldCreateCycle(1, 2, adj);
    CHECK(hasCycle == false);
  }
}

TEST_CASE("Story Graph - Large graph performance", "[story_graph][cycle][!benchmark]") {
  // Create a large DAG to test O(V+E) performance
  QSet<uint64_t> nodes;
  QHash<uint64_t, QList<uint64_t>> adj;

  const int numNodes = 1000;
  for (int i = 1; i <= numNodes; ++i) {
    nodes.insert(i);
    if (i < numNodes) {
      // Each node connects to next node (linear chain)
      adj[i].append(i + 1);
    }
  }

  SECTION("Large DAG has no cycles") {
    auto cycles = detectCycles(nodes, adj);
    CHECK(cycles.isEmpty());
  }

  SECTION("Cycle check at end of large chain") {
    // This tests the worst case where we need to traverse the entire chain
    bool hasCycle = wouldCreateCycle(numNodes, 1, adj);
    CHECK(hasCycle == true);
  }

  SECTION("No cycle when adding parallel edge") {
    bool hasCycle = wouldCreateCycle(500, 750, adj);
    CHECK(hasCycle == false);
  }
}
