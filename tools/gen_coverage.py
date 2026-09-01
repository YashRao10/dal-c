#!/usr/bin/env python3
"""
gen_coverage.py -- build docs/coverage/ from the gcov output of `make coverage`.

Reads build/gcov/*.c.gcov and writes an index plus one annotated source page
per file: a line gutter coloured by hit / miss, the hit count, and the
inline condition-coverage notes gcov emits with --conditions. No external
dependency (replaces gcovr). Exits non-zero if any src/ file is below 100%
line or condition coverage.

Python 3, standard library only.
"""
from __future__ import annotations

import html
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GCOV_DIR = ROOT / "build" / "gcov"
OUT_DIR = ROOT / "docs" / "coverage"

LINE_RE = re.compile(r"^ *([#=\-0-9]+) *: *(\d+):(.*)$")
COND_RE = re.compile(r"^condition outcomes covered (\d+)/(\d+)$")
NOTE_RE = re.compile(r"^(condition|branch|call) .+$")

_STYLE = """
:root{--bg:#07090c;--panel:#0c1015;--line:#1b232c;--ink:#c6d0d8;--ink-hi:#eef3f6;
 --ink-lo:#6c7a87;--amber:#f2b13c;--amber-dim:#8a6a2c;--green:#38d17a;--red:#e2564a;
 --mono:"IBM Plex Mono",ui-monospace,monospace;--sans:"IBM Plex Sans",system-ui,sans-serif;color-scheme:dark}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);font-family:var(--sans);font-size:14px;
 background-image:linear-gradient(var(--line) 1px,transparent 1px),linear-gradient(90deg,var(--line) 1px,transparent 1px);
 background-size:26px 26px;background-position:-1px -1px}
.wrap{max-width:1000px;margin:0 auto;padding:40px 22px 72px}
.mark{font-family:var(--mono);font-weight:600;color:var(--ink-hi);font-size:14px;display:flex;align-items:center;gap:8px;margin-bottom:24px}
.mark a{color:var(--ink-lo);text-decoration:none;font-size:11px;letter-spacing:.09em;text-transform:uppercase;margin-left:auto}
.eyebrow{font-family:var(--mono);font-size:11px;letter-spacing:.14em;text-transform:uppercase;color:var(--amber);margin:0 0 10px}
h1{font-family:var(--mono);font-size:19px;font-weight:600;margin:0 0 18px;color:var(--ink-hi)}
table{width:100%;border-collapse:collapse;font-family:var(--mono);font-size:12.5px}
th,td{text-align:left;padding:9px 11px;border-bottom:1px solid var(--line)}
th{color:var(--amber);font-weight:500;font-size:10.5px;text-transform:uppercase;letter-spacing:.06em;border-bottom:1px solid var(--amber-dim)}
td a{color:#8fb9d4;text-decoration:none}
.ok{color:var(--green);font-weight:600}
.bad{color:var(--red);font-weight:600}
pre.src{font-family:var(--mono);font-size:12px;line-height:1.55;background:var(--panel);
 border:1px solid var(--line);border-radius:8px;padding:14px 0;overflow-x:auto;margin:0}
pre.src .row{display:block;padding:0 14px;white-space:pre}
pre.src .hit{border-left:2px solid var(--green)}
pre.src .miss{border-left:2px solid var(--red);background:rgba(226,86,74,.07)}
pre.src .dim{border-left:2px solid transparent;color:#55606b}
pre.src .ln{color:#4b5560;display:inline-block;width:44px;text-align:right;margin-right:14px;user-select:none}
pre.src .ct{color:#6b7a87;display:inline-block;width:64px;text-align:right;margin-right:16px}
pre.src .note{display:block;padding:0 14px 0 122px;color:var(--amber-dim);font-size:11px}
footer{margin-top:30px;color:var(--ink-lo);font-size:11px;font-family:var(--mono)}
"""

_MARK = (
    '<div class="mark"><svg width="24" height="16" viewBox="0 0 26 18" fill="none">'
    '<path d="M2 13 C2 5 8 3 13 9 C18 15 24 13 24 5" stroke="#f2b13c" stroke-width="2" '
    'stroke-linecap="round"/></svg> dal-c '
    '<a href="{back}">&larr; {backlabel}</a></div>'
)


def parse(path: Path):
    src_name = None
    lines = []          # (lineno:int, count:str, text:str, notes:list[str])
    cov_lines = 0
    tot_lines = 0
    cond_num = 0
    cond_den = 0
    cur = None
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        m = LINE_RE.match(raw)
        if m:
            cnt, lineno, text = m.group(1), int(m.group(2)), m.group(3)
            if lineno == 0:
                if text.startswith("Source:"):
                    src_name = text[len("Source:"):].strip()
                continue
            cur = [lineno, cnt, text, []]
            lines.append(cur)
            if cnt.strip("-") != "" and cnt != "-":
                tot_lines += 1
                if cnt not in ("#####", "====="):
                    cov_lines += 1
            continue
        mc = COND_RE.match(raw)
        if mc:
            cond_num += int(mc.group(1))
            cond_den += int(mc.group(2))
        if NOTE_RE.match(raw) and cur is not None:
            cur[3].append(raw)
    return src_name, lines, cov_lines, tot_lines, cond_num, cond_den


def pct(n, d):
    return 100.0 if d == 0 else round(100.0 * n / d, 2)


def file_page(name, lines):
    body = []
    for lineno, cnt, text, notes in lines:
        if cnt == "-":
            cls, ct = "dim", ""
        elif cnt in ("#####", "====="):
            cls, ct = "miss", "0"
        else:
            cls, ct = "hit", cnt.strip()
        body.append(
            f'<span class="row {cls}"><span class="ln">{lineno}</span>'
            f'<span class="ct">{ct}</span>{html.escape(text)}</span>'
        )
        for note in notes:
            body.append(f'<span class="note">{html.escape(note)}</span>')
    return (
        "<!doctype html><html lang=en><head><meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        f"<title>dal-c coverage &mdash; {html.escape(name)}</title>"
        '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>'
        '<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600'
        '&family=IBM+Plex+Sans:wght@400;500&display=swap" rel="stylesheet">'
        f"<style>{_STYLE}</style></head><body><div class=wrap>"
        + _MARK.format(back="index.html", backlabel="coverage")
        + '<p class="eyebrow">&sect; Coverage</p>'
        + f"<h1>{html.escape(name)}</h1>"
        + f'<pre class="src">{"".join(body)}</pre>'
        + "<footer>dal-c &middot; tools/gen_coverage.py</footer></div></body></html>"
    )


def main() -> int:
    if not GCOV_DIR.is_dir():
        sys.exit(f"gen_coverage: {GCOV_DIR.relative_to(ROOT)} missing -- run `make coverage`")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    rows = []
    tot_c = tot_t = tot_cn = tot_cd = 0
    failed = []

    for g in sorted(GCOV_DIR.glob("*.c.gcov")):
        src, lines, cov, tot, cn, cd = parse(g)
        if not src or not src.startswith("src/"):
            continue
        lp, cp = pct(cov, tot), pct(cn, cd)
        page = src.split("/")[-1] + ".html"
        (OUT_DIR / page).write_text(file_page(src, lines), encoding="utf-8")
        rows.append((src, cov, tot, lp, cn, cd, cp, page))
        tot_c += cov; tot_t += tot; tot_cn += cn; tot_cd += cd
        if lp < 100.0 or cp < 100.0:
            failed.append(src)

    def cell(v, good):
        return f'<td class="{"ok" if good else "bad"}">{v}</td>'

    trs = "".join(
        f'<tr><td><a href="{p}">{html.escape(s)}</a></td>'
        f"<td>{cov}/{tot}</td>" + cell(f"{lp}%", lp == 100.0) +
        f"<td>{cn}/{cd}</td>" + cell(f"{cp}%", cp == 100.0) + "</tr>"
        for (s, cov, tot, lp, cn, cd, cp, p) in rows
    )
    olp, ocp = pct(tot_c, tot_t), pct(tot_cn, tot_cd)
    page = (
        "<!doctype html><html lang=en><head><meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>dal-c &mdash; Coverage Report</title>"
        '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>'
        '<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600'
        '&family=IBM+Plex+Sans:wght@400;500&display=swap" rel="stylesheet">'
        f"<style>{_STYLE}</style></head><body><div class=wrap>"
        + _MARK.format(back="../index.html", backlabel="overview")
        + '<p class="eyebrow">&sect; Coverage</p><h1>Coverage Report</h1>'
        + "<table><thead><tr><th>File</th><th>Lines</th><th>Line %</th>"
          "<th>Conditions</th><th>MC/DC %</th></tr></thead><tbody>"
        + trs
        + f'<tr><td><b>total</b></td><td>{tot_c}/{tot_t}</td>'
        + cell(f"{olp}%", olp == 100.0) + f"<td>{tot_cn}/{tot_cd}</td>"
        + cell(f"{ocp}%", ocp == 100.0) + "</tr>"
        + "</tbody></table>"
        + "<footer>dal-c &middot; statement + condition (MC/DC) coverage via GCC "
          "&middot; tools/gen_coverage.py</footer></div></body></html>"
    )
    (OUT_DIR / "index.html").write_text(page, encoding="utf-8")

    print(f"wrote {OUT_DIR.relative_to(ROOT)}/  "
          f"({tot_c}/{tot_t} lines {olp}%, {tot_cn}/{tot_cd} conditions {ocp}%)")
    if failed:
        print("BELOW 100%:", ", ".join(failed))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
