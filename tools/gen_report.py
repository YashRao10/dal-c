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
<style>
:root{{color-scheme:dark}}
body{{margin:0;background:#0b0f14;color:#c9d4de;font:14px/1.6 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}}
.wrap{{max-width:820px;margin:0 auto;padding:40px 20px 72px}}
h1{{font-size:22px;margin:0 0 4px;color:#e8eef4}}
h2{{font-size:13px;text-transform:uppercase;letter-spacing:.07em;color:#5cc2b8;margin:34px 0 12px;border-bottom:1px solid #1c2632;padding-bottom:6px}}
.sub{{color:#7c8b99;margin:0 0 20px;font-size:13px}}
table{{width:100%;border-collapse:collapse;font-size:13px}}
th,td{{text-align:left;padding:8px 10px;border-bottom:1px solid #1c2632}}
th{{color:#8fa2b3;font-weight:600;font-size:11px;text-transform:uppercase;letter-spacing:.05em}}
code{{background:#131c26;padding:1px 5px;border-radius:3px;color:#a8c7dd;font-size:12px}}
.ok{{color:#54c085;font-weight:600}}
.bad{{color:#e2705c;font-weight:600}}
.clean{{background:#12241c;border:1px solid #1f5138;color:#54c085;padding:12px 16px;border-radius:6px;font-weight:600}}
.gaps{{background:#241615;border:1px solid #5a2a22;color:#e2705c;padding:12px 16px;border-radius:6px;font-weight:600}}
.kv{{display:flex;gap:26px;flex-wrap:wrap;margin:14px 0 0;color:#8695a4;font-size:13px}}
.kv b{{color:#e8eef4}}
footer{{margin-top:36px;color:#5c6b78;font-size:12px}}
a{{color:#7db8dd}}
</style></head><body><div class="wrap">
<h1>dal-c &mdash; Verification Results</h1>
<p class="sub">Generated from a real <code>make coverage</code> run. Statement and
condition (MC/DC) coverage measured with GCC; requirements trace in the
<a href="traceability.html">traceability matrix</a>.</p>

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

<footer>dal-c &middot; generated by tools/gen_report.py</footer>
</div></body></html>
"""


if __name__ == "__main__":
    sys.exit(main())
