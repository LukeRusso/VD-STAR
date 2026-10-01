from __future__ import annotations

import pytest
import vdstar

N = 6
EDGES = [(1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (1, 6)]
RHO = 0.01


def build_graph() -> vdstar.Graph:
    g = vdstar.Graph(N, RHO)
    for u, v in EDGES:
        g.insert_edge(u, v)
    return g


def test_roundtrip_is_byte_identical(tmp_path):
    eps = 0.5
    mu = 2
    g = build_graph()
    before = g.cluster(eps, mu)

    first = tmp_path / "state.bin"
    second = tmp_path / "state2.bin"

    vdstar.GraphStore.write(g, str(first))

    h = vdstar.GraphStore.read(str(first))
    after = h.cluster(eps, mu)
    assert before == after

    vdstar.GraphStore.write(h, str(second))
    a = first.read_bytes()
    b = second.read_bytes()
    assert a == b, "save -> load -> save is not byte-identical"
    assert len(a) > 0, "state file is empty"


def test_foreign_file_is_rejected(tmp_path):
    """A foreign file must raise RuntimeError, not bad_alloc or a crash."""
    foreign = tmp_path / "foreign.bin"
    foreign.write_text("not an archive\n")

    with pytest.raises(RuntimeError, match="not a VD-STAR state file"):
        vdstar.GraphStore.read(str(foreign))


def test_missing_file_is_rejected(tmp_path):
    with pytest.raises(RuntimeError, match="cannot open"):
        vdstar.GraphStore.read(str(tmp_path / "nope.bin"))
