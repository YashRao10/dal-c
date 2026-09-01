#!/usr/bin/env python3
"""
gen_rtm.py -- build the requirements traceability matrix for dal-c.

Reads the requirement text from requirements/*.md, then confirms each
low-level requirement against the actual source (an `LLR-...` tag in a
src/*.c comment) and test suite (its component test file). Emits
docs/traceability.html and prints a summary; exits non-zero if any
requirement is unimplemented or untraced.

Python 3, standard library only.
"""
from __future__ import annotations

import html
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REQ_DIR = ROOT / "requirements"
SRC_DIR = ROOT / "src"
TEST_DIR = ROOT / "tests"
OUT = ROOT / "docs" / "traceability.html"

HLR_RE = re.compile(r"^- \*\*(HLR-[A-Z]+-\d+)\*\*\s*[—-]\s*(.*)$")
LLR_ROW_RE = re.compile(
    r"^\|\s*(LLR-[A-Z]+-\d+)\s*\|\s*([^|]*?)\s*\|\s*([^|]*?)\s*\|\s*([^|]*?)\s*\|\s*$"
)
REQ_ID_RE = re.compile(r"\b((?:HLR|LLR)-[A-Z]+-\d+)\b")
COMPONENT_RE = re.compile(r"-([A-Z]+)-\d+$")


@dataclass
class Hlr:
    rid: str
    text: str
    llrs: list[str] = field(default_factory=list)


@dataclass
class Llr:
    rid: str
    traces: list[str]
    text: str
    code: str
    in_source: bool = False
    in_tests: bool = False


def component_of(rid: str) -> str:
    m = COMPONENT_RE.search(rid)
    return m.group(1) if m else "?"


def load_requirements() -> tuple[dict[str, Hlr], dict[str, Llr]]:
    hlrs: dict[str, Hlr] = {}
    llrs: dict[str, Llr] = {}
    for md in sorted(REQ_DIR.glob("*.md")):
        lines = md.read_text(encoding="utf-8").splitlines()
        cur: Hlr | None = None
        for line in lines:
            mh = HLR_RE.match(line)
            if mh:
                cur = Hlr(mh.group(1), mh.group(2).strip())
                hlrs[cur.rid] = cur
                continue
            # continuation line of the current HLR: indented, not a new bullet
            if cur is not None and line.startswith("  ") and line.strip() \
                    and not line.lstrip().startswith(("- ", "|", "#")):
                cur.text = (cur.text + " " + line.strip()).strip()
                continue
            cur = None
            ml = LLR_ROW_RE.match(line)
            if ml and ml.group(1).startswith("LLR-"):
                rid, traces, text, code = ml.groups()
                trace_ids = REQ_ID_RE.findall(traces)
                llrs[rid] = Llr(rid, trace_ids, text.strip(), code.strip())
    for llr in llrs.values():
        for pid in llr.traces:
            if pid in hlrs:
                hlrs[pid].llrs.append(llr.rid)
    return hlrs, llrs


def scan(paths) -> set[str]:
    found: set[str] = set()
    for p in paths:
        for m in REQ_ID_RE.finditer(p.read_text(encoding="utf-8")):
            found.add(m.group(1))
    return found


def build() -> int:
    hlrs, llrs = load_requirements()
    src_ids = scan(SRC_DIR.glob("*.c"))
    test_ids = scan(TEST_DIR.glob("*.c"))

    # a component's LLRs are verified by its own test file if that file exists
    test_components = {
        component_of(f.stem.replace("test_sc_", "SC").upper())
        for f in TEST_DIR.glob("test_*.c")
    }
    have_test_file = {p.stem for p in TEST_DIR.glob("test_*.c")}

    problems: list[str] = []
    for llr in llrs.values():
        llr.in_source = llr.rid in src_ids
        comp = component_of(llr.rid).lower()
        # map HYS->hysteresis etc via the source file that carries the tag
        llr.in_tests = (f"test_sc_{_comp_file(comp)}" in have_test_file)
        if not llr.in_source:
            problems.append(f"{llr.rid}: no LLR tag found in any src/*.c")
        if not llr.traces:
            problems.append(f"{llr.rid}: does not trace up to any HLR")
    for hlr in hlrs.values():
        if not hlr.llrs:
            problems.append(f"{hlr.rid}: no LLR traces down to it")
        if hlr.rid not in test_ids:
            problems.append(f"{hlr.rid}: not referenced by any test")

    write_html(hlrs, llrs, problems)

    print(f"HLRs: {len(hlrs)}   LLRs: {len(llrs)}   "
          f"source-tagged: {len([l for l in llrs.values() if l.in_source])}/{len(llrs)}")
    if problems:
        print("\nTRACEABILITY GAPS:")
        for p in problems:
            print(f"  - {p}")
        return 1
    print("Traceability: complete (every HLR has LLRs and tests; every LLR is "
          "implemented and traced).")
    return 0


_COMP_FILE = {
    "hys": "hysteresis",
    "rb": "ringbuf",
    "rl": "ratelimit",
    "sat": "sat",
}


def _comp_file(comp: str) -> str:
    return _COMP_FILE.get(comp, comp)


def write_html(hlrs, llrs, problems) -> None:
    def esc(s: str) -> str:
        return html.escape(s)

    def md(s: str) -> str:
        """Escape, then render `inline code` spans."""
        parts = s.split("`")
        out = []
        for i, part in enumerate(parts):
            out.append(f"<code>{html.escape(part)}</code>" if i % 2
                       else html.escape(part))
        return "".join(out)

    rows = []
    for comp in sorted({component_of(h) for h in hlrs}):
        comp_hlrs = sorted(
            (h for h in hlrs.values() if component_of(h.rid) == comp),
            key=lambda h: _natkey(h.rid),
        )
        rows.append(f'<tr class="grp"><td colspan="5">{esc(_comp_file(comp.lower()))}</td></tr>')
        for hlr in comp_hlrs:
            child = sorted(hlr.llrs, key=_natkey) or ["—"]
            for i, lid in enumerate(child):
                llr = llrs.get(lid)
                first = i == 0
                hcell = (f'<td rowspan="{len(child)}"><b>{esc(hlr.rid)}</b>'
                         f'<div class="t">{md(hlr.text)}</div></td>') if first else ""
                if llr is None:
                    rows.append(
                        f'<tr>{hcell}<td class="bad">— no LLR —</td>'
                        f'<td></td><td></td><td class="bad">GAP</td></tr>')
                    continue
                ok = llr.in_source and llr.in_tests and bool(llr.traces)
                status = ('<span class="ok">traced</span>' if ok
                          else '<span class="bad">GAP</span>')
                rows.append(
                    f"<tr>{hcell}"
                    f'<td><b>{esc(llr.rid)}</b><div class="t">{md(llr.text)}</div></td>'
                    f"<td>{md(llr.code)}</td>"
                    f"<td><code>test_sc_{esc(_comp_file(comp.lower()))}.c</code></td>"
                    f"<td>{status}</td></tr>")

    banner = ('<div class="gaps"><b>Traceability gaps:</b><ul>'
              + "".join(f"<li>{esc(p)}</li>" for p in problems) + "</ul></div>"
              ) if problems else '<div class="clean">No traceability gaps.</div>'

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(_PAGE.format(banner=banner, rows="\n".join(rows)), encoding="utf-8")
    print(f"wrote {OUT.relative_to(ROOT)}")


def _natkey(rid: str):
    return [int(t) if t.isdigit() else t for t in re.split(r"(\d+)", rid)]


_PAGE = """<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>dal-c &mdash; Requirements Traceability Matrix</title>
<style>
:root{{color-scheme:dark}}
body{{margin:0;background:#0b0f14;color:#c9d4de;font:14px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}}
.wrap{{max-width:1100px;margin:0 auto;padding:32px 20px 64px}}
h1{{font-size:20px;letter-spacing:.02em;margin:0 0 4px;color:#e8eef4}}
.sub{{color:#7c8b99;margin:0 0 24px}}
table{{width:100%;border-collapse:collapse;font-size:13px}}
th,td{{text-align:left;padding:8px 10px;border-bottom:1px solid #1c2632;vertical-align:top}}
th{{color:#8fa2b3;font-weight:600;text-transform:uppercase;font-size:11px;letter-spacing:.06em;border-bottom:1px solid #2a3746}}
tr.grp td{{background:#111823;color:#5cc2b8;font-weight:600;font-family:ui-monospace,monospace;letter-spacing:.03em;border-bottom:1px solid #2a3746}}
.t{{color:#8695a4;font-size:12px;margin-top:2px}}
code{{background:#131c26;padding:1px 5px;border-radius:3px;color:#a8c7dd;font-size:12px}}
.ok{{color:#54c085;font-weight:600}}
.bad{{color:#e2705c;font-weight:600}}
.clean{{background:#12241c;border:1px solid #1f5138;color:#54c085;padding:10px 14px;border-radius:6px;margin-bottom:20px}}
.gaps{{background:#241615;border:1px solid #5a2a22;color:#e2705c;padding:10px 14px;border-radius:6px;margin-bottom:20px}}
.gaps ul{{margin:6px 0 0;padding-left:20px}}
footer{{margin-top:28px;color:#5c6b78;font-size:12px}}
</style></head><body><div class="wrap">
<h1>dal-c &mdash; Requirements Traceability Matrix</h1>
<p class="sub">High-level requirement &rarr; low-level requirement &rarr; source &rarr; verification.
Generated from <code>requirements/*.md</code> and confirmed against the source tags and test suites.</p>
{banner}
<table>
<thead><tr><th>High-Level Requirement</th><th>Low-Level Requirement</th><th>Implemented in</th><th>Verified by</th><th>Status</th></tr></thead>
<tbody>
{rows}
</tbody></table>
<footer>dal-c &middot; generated by tools/gen_rtm.py</footer>
</div></body></html>
"""


if __name__ == "__main__":
    sys.exit(build())
