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
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600&family=IBM+Plex+Sans:wght@400;500&display=swap" rel="stylesheet">
<style>
:root{{--bg:#07090c;--panel:#0c1015;--line:#1b232c;--ink:#c6d0d8;--ink-hi:#eef3f6;
  --ink-lo:#6c7a87;--amber:#f2b13c;--amber-dim:#8a6a2c;--green:#38d17a;--red:#e2564a;
  --mono:"IBM Plex Mono",ui-monospace,monospace;--sans:"IBM Plex Sans",system-ui,sans-serif;color-scheme:dark}}
*{{box-sizing:border-box}}
body{{margin:0;background:var(--bg);color:var(--ink);font-family:var(--sans);font-size:14px;
  background-image:linear-gradient(var(--line) 1px,transparent 1px),linear-gradient(90deg,var(--line) 1px,transparent 1px);
  background-size:26px 26px;background-position:-1px -1px}}
.wrap{{max-width:1080px;margin:0 auto;padding:40px 22px 72px}}
.mark{{font-family:var(--mono);font-weight:600;color:var(--ink-hi);font-size:14px;
  display:flex;align-items:center;gap:8px;margin-bottom:26px}}
.mark svg{{display:block}}
.mark a{{color:var(--ink-lo);text-decoration:none;font-size:11px;letter-spacing:.09em;text-transform:uppercase;margin-left:auto}}
h1{{font-family:var(--mono);font-size:19px;font-weight:600;letter-spacing:.01em;margin:0 0 4px;color:var(--ink-hi)}}
.sub{{color:var(--ink-lo);margin:0 0 22px;max-width:64ch}}
.eyebrow{{font-family:var(--mono);font-size:11px;letter-spacing:.14em;text-transform:uppercase;color:var(--amber);margin:0 0 10px}}
table{{width:100%;border-collapse:collapse;font-size:12.5px;font-family:var(--mono)}}
th,td{{text-align:left;padding:9px 11px;border-bottom:1px solid var(--line);vertical-align:top}}
th{{color:var(--amber);font-weight:500;text-transform:uppercase;font-size:10.5px;letter-spacing:.07em;border-bottom:1px solid var(--amber-dim)}}
tr.grp td{{background:var(--panel);color:var(--ink-hi);font-weight:600;letter-spacing:.04em;
  border-top:1px solid var(--line);border-bottom:1px solid var(--line)}}
.t{{font-family:var(--sans);color:var(--ink-lo);font-size:12.5px;margin-top:3px}}
code{{background:#11161c;border:1px solid var(--line);padding:1px 5px;border-radius:3px;color:#a9c0cf;font-size:11.5px}}
.ok{{color:var(--green);font-weight:600}}
.bad{{color:var(--red);font-weight:600}}
.clean{{font-family:var(--mono);background:#0c1a13;border:1px solid #1f5138;color:var(--green);
  padding:11px 15px;border-radius:6px;margin-bottom:22px;font-size:12.5px;letter-spacing:.02em}}
.gaps{{font-family:var(--mono);background:#1c1210;border:1px solid #5a2a22;color:var(--red);
  padding:11px 15px;border-radius:6px;margin-bottom:22px;font-size:12.5px}}
.gaps ul{{margin:6px 0 0;padding-left:20px}}
footer{{margin-top:30px;color:var(--ink-lo);font-size:11px;font-family:var(--mono)}}
</style></head><body><div class="wrap">
<div class="mark">
  <svg width="24" height="16" viewBox="0 0 26 18" fill="none"><path d="M2 13 C2 5 8 3 13 9 C18 15 24 13 24 5" stroke="#f2b13c" stroke-width="2" stroke-linecap="round"/></svg>
  dal-c library <a href="index.html">&larr; overview</a>
</div>
<p class="eyebrow">&sect; Traceability</p>
<h1>Requirements Traceability Matrix</h1>
<p class="sub">High-level requirement &rarr; low-level requirement &rarr; source function &rarr;
verifying suite. Generated from <code>requirements/*.md</code> and confirmed against the
<code>/* LLR-... */</code> source tags and the test files.</p>
{banner}
<table>
<thead><tr><th>High-Level Requirement</th><th>Low-Level Requirement</th><th>Implemented in</th><th>Verified by</th><th>Status</th></tr></thead>
<tbody>
{rows}
</tbody></table>
<footer>dal-c &middot; tools/gen_rtm.py</footer>
</div></body></html>
"""


if __name__ == "__main__":
    sys.exit(build())
