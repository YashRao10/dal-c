#!/usr/bin/env python3
"""
gen_report.py -- build docs/verification.html from a real coverage run.

Reads build/test-output.txt and build/coverage-summary.txt (produced by
`make coverage`) and writes a Software-Verification-Results style page: the
per-suite test result and the per-file structural coverage, with an overall
verdict. Exits non-zero if the inputs are missing or the verdict is not a
full pass.

Python 3, standard library only.
"""
from __future__ import annotations

import html
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
OUT = ROOT / "docs" / "verification.html"

SUITE_RE = re.compile(r"^suite\t([^\t]+)\t(\d+)$")
TOTAL_RE = re.compile(r"^total\t(\d+)\t(\d+)$")
FILE_RE = re.compile(r"^File '(.+?)'$")
LINES_RE = re.compile(r"^Lines executed:([\d.]+)% of (\d+)$")
COND_RE = re.compile(r"^Condition outcomes covered:([\d.]+)% of (\d+)$")


def read(name: str) -> str:
    p = BUILD / name
    if not p.exists():
        sys.exit(f"gen_report: missing {p.relative_to(ROOT)} -- run `make coverage` first")
    return p.read_text(encoding="utf-8")


def parse_tests(text: str):
    suites, total = [], None
    for line in text.splitlines():
        m = SUITE_RE.match(line)
        if m:
            suites.append((m.group(1), int(m.group(2))))
        m = TOTAL_RE.match(line)
        if m:
            total = (int(m.group(1)), int(m.group(2)))
    # per-suite check counts from the human table ("  sc_sat  8 tests  31 checks  PASS")
    checks = {}
    for line in text.splitlines():
        m = re.match(r"^\s+(\S+)\s+\d+ tests\s+(\d+) checks\s+(PASS|FAIL)$", line)
        if m:
            checks[m.group(1)] = (int(m.group(2)), m.group(3))
    rows = [(name, n, *checks.get(name, (0, "?"))) for name, n in suites]
    return rows, total


def parse_coverage(text: str):
    files, cur = [], None
    for line in text.splitlines():
        m = FILE_RE.match(line)
        if m:
            cur = {"file": m.group(1)}
            files.append(cur)
        elif cur is not None:
            m = LINES_RE.match(line)
            if m and "lines" not in cur:
                cur["line_pct"], cur["lines"] = float(m.group(1)), int(m.group(2))
                continue
            m = COND_RE.match(line)
            if m and "conds" not in cur:
                cur["cond_pct"], cur["conds"] = float(m.group(1)), int(m.group(2))
                continue
            # a line that isn't this file's first Lines/Condition ends the block
            cur = None
    return [f for f in files if f["file"].startswith("src/")]


def main() -> int:
    tests_txt = read("test-output.txt")
    cov_txt = read("coverage-summary.txt")
    rows, total = parse_tests(tests_txt)
    files = parse_coverage(cov_txt)

    checks_total, fails_total = total if total else (0, 1)
    line_ok = all(abs(f.get("line_pct", 0) - 100.0) < 1e-9 for f in files) and files
    cond_ok = all(abs(f.get("cond_pct", 0) - 100.0) < 1e-9 for f in files) and files
    verdict_pass = (fails_total == 0) and line_ok and cond_ok

    total_lines = sum(f.get("lines", 0) for f in files)
    total_conds = sum(f.get("conds", 0) for f in files)

    trows = "".join(
        f"<tr><td><code>{html.escape(n)}</code></td><td>{ntests}</td>"
        f"<td>{nchecks}</td>"
        f'<td class="{ "ok" if res == "PASS" else "bad" }">{res}</td></tr>'
        for n, ntests, nchecks, res in rows
    )
    crows = "".join(
        f'<tr><td><code>{html.escape(f["file"])}</code></td>'
        f'<td>{f.get("lines","?")}</td>'
        f'<td class="{ "ok" if f.get("line_pct")==100.0 else "bad" }">{f.get("line_pct","?")}%</td>'
        f'<td>{f.get("conds","?")}</td>'
        f'<td class="{ "ok" if f.get("cond_pct")==100.0 else "bad" }">{f.get("cond_pct","?")}%</td></tr>'
        for f in files
    )

    verdict = ('<div class="clean">PASS &mdash; all requirements-based tests '
               'passed; 100% statement and 100% condition (MC/DC) coverage.</div>'
               if verdict_pass else
               '<div class="gaps">NOT A FULL PASS &mdash; see the tables below.</div>')

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(_PAGE.format(
        verdict=verdict,
        when=datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC"),
        checks=checks_total, fails=fails_total,
        total_lines=total_lines, total_conds=total_conds,
        trows=trows, crows=crows,
    ), encoding="utf-8")
    print(f"wrote {OUT.relative_to(ROOT)}  "
          f"({checks_total} checks, {fails_total} fail; "
          f"{total_lines} lines / {total_conds} conditions)")
    return 0 if verdict_pass else 1


_PAGE = """<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>dal-c &mdash; Verification Results</title>
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
.wrap{{max-width:840px;margin:0 auto;padding:40px 22px 72px}}
.mark{{font-family:var(--mono);font-weight:600;color:var(--ink-hi);font-size:14px;
  display:flex;align-items:center;gap:8px;margin-bottom:26px}}
.mark a{{color:var(--ink-lo);text-decoration:none;font-size:11px;letter-spacing:.09em;text-transform:uppercase;margin-left:auto}}
.eyebrow{{font-family:var(--mono);font-size:11px;letter-spacing:.14em;text-transform:uppercase;color:var(--amber);margin:0 0 10px}}
h1{{font-family:var(--mono);font-size:19px;font-weight:600;margin:0 0 4px;color:var(--ink-hi)}}
.sub{{color:var(--ink-lo);margin:0 0 20px;font-size:13px;max-width:60ch}}
h2{{font-family:var(--mono);font-size:12px;text-transform:uppercase;letter-spacing:.1em;
  color:var(--ink-lo);margin:32px 0 12px;border-bottom:1px solid var(--line);padding-bottom:7px}}
h2::before{{content:"\\00a7\\00a0";color:var(--amber)}}
table{{width:100%;border-collapse:collapse;font-size:12.5px;font-family:var(--mono)}}
th,td{{text-align:left;padding:9px 11px;border-bottom:1px solid var(--line)}}
th{{color:var(--amber);font-weight:500;font-size:10.5px;text-transform:uppercase;letter-spacing:.06em;border-bottom:1px solid var(--amber-dim)}}
code{{background:#11161c;border:1px solid var(--line);padding:1px 5px;border-radius:3px;color:#a9c0cf;font-size:11.5px}}
.ok{{color:var(--green);font-weight:600}}
.bad{{color:var(--red);font-weight:600}}
.clean{{font-family:var(--mono);background:#0c1a13;border:1px solid #1f5138;color:var(--green);
  padding:13px 16px;border-radius:6px;font-weight:600;font-size:12.5px;letter-spacing:.02em}}
.gaps{{font-family:var(--mono);background:#1c1210;border:1px solid #5a2a22;color:var(--red);
  padding:13px 16px;border-radius:6px;font-weight:600;font-size:12.5px}}
.kv{{display:flex;gap:8px;flex-wrap:wrap;margin:14px 0 0;font-family:var(--mono);font-size:11px}}
.kv span{{background:var(--panel);border:1px solid var(--line);border-radius:4px;padding:6px 10px;color:var(--ink-lo);letter-spacing:.04em}}
.kv b{{color:var(--amber);font-weight:500}}
footer{{margin-top:36px;color:var(--ink-lo);font-size:11px;font-family:var(--mono)}}
a{{color:#8fb9d4}}
</style></head><body><div class="wrap">
<div class="mark">
  <svg width="24" height="16" viewBox="0 0 26 18" fill="none"><path d="M2 13 C2 5 8 3 13 9 C18 15 24 13 24 5" stroke="#f2b13c" stroke-width="2" stroke-linecap="round"/></svg>
  dal-c library <a href="index.html">&larr; overview</a>
</div>
<p class="eyebrow">&sect; Verification</p>
<h1>Verification Results</h1>
<p class="sub">Generated from a real <code>make coverage</code> run &mdash; not
hand-maintained. Statement and condition (MC/DC) coverage measured with GCC;
requirements trace in the <a href="traceability.html">traceability matrix</a>.</p>

{verdict}
<div class="kv">
  <span>run <b>{when}</b></span>
  <span>checks <b>{checks}</b></span>
  <span>failures <b>{fails}</b></span>
  <span>lines <b>{total_lines}</b></span>
  <span>conditions <b>{total_conds}</b></span>
</div>

<h2>Requirements-based tests</h2>
<table><thead><tr><th>Suite</th><th>Tests</th><th>Checks</th><th>Result</th></tr></thead>
<tbody>{trows}</tbody></table>

<h2>Structural coverage</h2>
<table><thead><tr><th>File</th><th>Lines</th><th>Line %</th><th>Conditions</th><th>MC/DC %</th></tr></thead>
<tbody>{crows}</tbody></table>

<footer>dal-c &middot; tools/gen_report.py</footer>
</div></body></html>
"""


if __name__ == "__main__":
    sys.exit(main())
