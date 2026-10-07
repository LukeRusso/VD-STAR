#include "MyLib/MyVector.h"
#include "graph/Graph.h"
#include "graph/GraphStore.h"
#include "graph/Vertex.h"
#include <optional>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <sstream>

namespace py = pybind11;

std::vector<std::vector<int>>
cluster(int n, const std::vector<std::pair<int, int>> &edges, double eps,
        int mu, double rho) {
  Graph graph(n, rho);

  for (int i = 0; i < edges.size(); i++) {
    graph.insertEdge(edges[i].first, edges[i].second);
  }

  return graph.query(eps, mu);
}

PYBIND11_MODULE(vdstar_core, m) {
  m.doc() = "VD-STAR dynamic structural clustering";

  py::class_<Graph>(m, "Graph", R"doc(
        A persistent VD-STAR graph.

        Mutate with insert_edge / remove_edge / add_vertex / remove_vertex,
        then query with cluster(eps, mu).

        Vertex ids are 1-based, assigned by vdstar, and never reused after a
        vertex is removed.
        )doc")
      .def(py::init<int, double, std::optional<unsigned long long>,
                    std::optional<int>>(),
           py::arg("n"), py::arg("rho") = 0.01, py::arg("seed") = py::none(),
           py::arg("permutation_num") = py::none(),
           R"doc(Create a graph with n vertices, ids 1..n.

             rho:  approximation budget in (0, 1].
             seed: sampler seed, or None to seed randomly from the system.
             permutation_num: large/small degree threshold, or None to derive
             it from rho and n.
             )doc")
      .def(
          "add_vertex", [](Graph &g) { return g.addVertex()->id; },
          R"doc(Add a vertex. Returns the id vdstar assigned.
             )doc")
      .def("get_vertex_num", &Graph::getVertexNum,
           R"doc(The number of vertex slots, including slots left by removed
             vertices. Ids are 1-based, so the next id vdstar will assign is
             one past this count.
             )doc")
      .def(
          "insert_edge", [](Graph &g, int u, int v) { g.insertEdge(u, v); },
          py::arg("u"), py::arg("v"),
          "Insert an edge between two existing vertices.")
      .def(
          "remove_edge", [](Graph &g, int u, int v) { g.removeEdge(u, v); },
          py::arg("u"), py::arg("v"), "Remove an edge.")
      .def("remove_vertex", &Graph::removeVertex, py::arg("v"),
           "Remove a vertex and its incident edges. True if a real vertex was "
           "removed.")
      .def("cluster", &Graph::query, py::arg("eps"), py::arg("mu"),
           R"doc(Cluster the graph at its current state.

             Returns: list[list[int]] of clusters, each cluster is a list of
             1-based vertex ids.
             )doc");

  py::class_<GraphStore>(m, "GraphStore", R"doc(
        Read and write VD-STAR graph states.

        write(graph, path) saves a graph; read(path) returns a new Graph.
        )doc")
      .def_static("write",
                  py::overload_cast<const Graph &, const std::string &>(
                      &GraphStore::write),
                  py::arg("graph"), py::arg("path"),
                  R"doc(Save graph to path.

                     Raises RuntimeError if the path cannot be written.
                     )doc")
      .def_static(
          "read", py::overload_cast<const std::string &>(&GraphStore::read),
          py::arg("path"),
          R"doc(Read a graph state previously written. Returns a new Graph.

             Raises RuntimeError if path is not a VD-STAR state file.
             )doc")
      .def_static(
          "write_bytes",
          [](const Graph &g) {
            std::ostringstream os(std::ios::binary);
            GraphStore::write(os, g);
            return py::bytes(os.str());
          },
          py::arg("graph"),
          R"doc(Serialise graph to bytes without touching the filesystem.

             Returns: bytes holding the same archive format write() produces.
             )doc")
      .def_static(
          "read_bytes",
          [](const py::bytes &data) {
            std::istringstream is(std::string(data), std::ios::binary);
            return GraphStore::read(is);
          },
          py::arg("data"),
          R"doc(Read a graph state from bytes previously produced by write_bytes.

             Raises RuntimeError if data is not a VD-STAR state archive.
             )doc");

  m.def(
      "cluster", &cluster,
      R"doc(Cluster an unweighted graph by VD-STAR dynamic structural clustering.

        Args:
            n:     number of vertices (1..n). Edge endpoint ids are 1-indexed integers.
            edges: list of (source, target) vertex-id pairs, each in [1, n].
            eps:   similarity threshold in (0, 1). Default-candidates.
            mu:    min number of eps-similar neighbors to be a core (>=1).
            rho:   approximation budget in (0, 1], default 0.01.

        Returns: list[list[int]] of clusters; each cluster is a list of 1-based vertex ids.
        )doc",
      py::arg("n"), py::arg("edges"), py::arg("eps"), py::arg("mu"),
      py::arg("rho") = 0.01);
}