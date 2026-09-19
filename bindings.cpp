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

    MyVector<dynscan::Vertex *> _vList;
    _vList.reserve(n);

    for (int i = 0; i < n; i++) {
        dynscan::Vertex *newVertex = new dynscan::Vertex(i + 1);
        _vList.push_back(newVertex);
    }
    Graph graph(std::move(_vList), rho);

    for (int i = 0; i < edges.size(); i++) {
        graph.insertEdge(edges[i].first, edges[i].second);
    }

    return graph.query(eps, mu);
}

PYBIND11_MODULE(vdstar_core, m) {
    m.doc() = "VD-STAR dynamic structural clustering";
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