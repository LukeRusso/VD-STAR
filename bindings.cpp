#include <optional>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "graph/Graph.h"
#include "graph/Vertex.h"
#include "MyLib/MyVector.h"

namespace py = pybind11;

std::vector<std::vector<int>> cluster(
        int n,
        const std::vector<std::pair<int, int>>& edges,
        double eps, int mu, double rho) {
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
        .def(py::init<int, double, std::optional<unsigned long long>>(),
             py::arg("n"), py::arg("rho") = 0.01, py::arg("seed") = py::none(),
             R"doc(Create a graph with n vertices, ids 1..n.

             rho:  approximation budget in (0, 1].
             seed: sampler seed, or None to seed randomly from the system.
             )doc")
        .def("add_vertex", [](Graph &g) { return g.addVertex()->id; },
             R"doc(Add a vertex. Returns the id vdstar assigned.
             )doc")
        .def("insert_edge", [](Graph &g, int u, int v) { g.insertEdge(u, v); },
             py::arg("u"), py::arg("v"),
             "Insert an edge between two existing vertices.")
        .def("remove_edge", [](Graph &g, int u, int v) { g.removeEdge(u, v); },
             py::arg("u"), py::arg("v"),
             "Remove an edge.")
        .def("remove_vertex", &Graph::removeVertex, py::arg("v"),
             "Remove a vertex and its incident edges. True if a real vertex was removed.")
        .def("cluster", &Graph::query, py::arg("eps"), py::arg("mu"),
             R"doc(Cluster the graph at its current state.

             Returns: list[list[int]] of clusters, each cluster is a list of
             1-based vertex ids.
             )doc");

    m.def("cluster", &cluster, R"doc(Cluster an unweighted graph by VD-STAR dynamic structural clustering.

        Args:
            n:     number of vertices (1..n). Edge endpoint ids are 1-indexed integers.
            edges: list of (source, target) vertex-id pairs, each in [1, n].
            eps:   similarity threshold in (0, 1). Default-candidates.
            mu:    min number of eps-similar neighbors to be a core (>=1).
            rho:   approximation budget in (0, 1], default 0.01.

        Returns: list[list[int]] of clusters; each cluster is a list of 1-based vertex ids.
        )doc",
        py::arg("n"), py::arg("edges"),
        py::arg("eps"), py::arg("mu"), py::arg("rho") = 0.01);
}