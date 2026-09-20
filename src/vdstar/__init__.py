"""VD-STAR dynamic structural clustering — Python wrapper."""

from __future__ import annotations

from collections.abc import Sequence

from vdstar_core import Graph
from vdstar_core import cluster as _cluster_core

__all__ = ["Graph", "cluster"]


def cluster(
    n: int,
    edges: Sequence[tuple[int, int]],
    eps: float,
    mu: int,
    rho: float = 0.01,
) -> list[list[int]]:
    """Cluster an unweighted graph using the VD-STAR engine.

    Parameters
    ----------
    n : int
        Number of vertices.
    edges : sequence of (int, int)
        Each edge is a (source, target) pair of 1-based vertex ids in [1, n].
    eps : float
        Structural-similarity threshold in (0, 1). An edge is "similar" if
        its similarity >= eps.
    mu : int
        Number of eps-similar neighbors a vertex needs to be a structural core
        (must be >= 1).
    rho : float
        Approximation budget in (0, 1] (default 0.01).

    Returns
    -------
    list[list[int]]
        Each inner list a cluster of 1-based vertex ids.
    """
    if not isinstance(n, int) or n <= 0:
        raise ValueError("n must be a positive integer")

    if not (rho > 0.0 and rho <= 1.0):
        raise ValueError("rho must be in (0, 1]")

    if not (eps > 0.0 and eps < 1.0):
        raise ValueError("eps must be in (0, 1)")

    if not (mu >= 1 and mu < n) or not isinstance(mu, int):
        raise ValueError("mu must be an integer in [1, n)")

    edge_list: list[tuple[int, int]] = []
    seen: set[tuple[int, int]] = set()
    for e in edges:
        if len(e) != 2:
            raise ValueError(f"edge must be a (source, target) pair, got {e!r}")
        u, v = int(e[0]), int(e[1])
        # normalise
        u, v = (u, v) if u <= v else (v, u)
        if not (1 <= u <= n and 1 <= v <= n):
            raise ValueError(f"edge {e!r}: vertex ids must be 1-based and in [1, n]")
        if u == v:
            raise ValueError("self-loop edge not supported")
        if (u, v) in seen:
            continue
        seen.add((u, v))
        edge_list.append((u, v))

    if not edge_list:
        return []

    return _cluster_core(n, edge_list, float(eps), int(mu), float(rho))
