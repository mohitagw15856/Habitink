"""Command-line entry point for the HabitInk companion."""
from __future__ import annotations

import argparse
import datetime as dt
import sys
from pathlib import Path

from . import __version__
from .edit import run_edit
from .model import DataStore
from .report import run_report


def _add_common(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--sd",
        default=".",
        help="path to the SD card root (or the .habitink directory directly)",
    )
    parser.add_argument(
        "--today",
        default=None,
        help="override today's date (YYYY-MM-DD), mainly for reproducible output",
    )


def _today(args: argparse.Namespace) -> dt.date:
    return dt.date.fromisoformat(args.today) if args.today else dt.date.today()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="habitink", description="HabitInk companion tool")
    parser.add_argument("--version", action="version", version=f"habitink {__version__}")
    sub = parser.add_subparsers(dest="command", required=True)

    report = sub.add_parser("report", help="produce a monthly Markdown report and PNG grid")
    _add_common(report)
    report.add_argument("--month", default=None, help="report month as YYYY-MM (default: current month)")
    report.add_argument("--out", default="habitink-report.md", help="output Markdown path")
    report.add_argument("--png", default="habitink-grid.png", help="output PNG path ('none' to skip)")

    edit = sub.add_parser("edit", help="backfill or correct a completion record")
    _add_common(edit)
    edit.add_argument("--habit", required=True, help="habit id or name")
    edit.add_argument("--date", required=True, help="date YYYY-MM-DD, or a range DATE..DATE")
    group = edit.add_mutually_exclusive_group()
    group.add_argument("--done", action="store_true", help="mark the day(s) completed (default)")
    group.add_argument("--clear", action="store_true", help="mark the day(s) not completed")

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    store = DataStore.locate(args.sd)

    if args.command == "report":
        out_md = Path(args.out)
        out_png = None if str(args.png).lower() == "none" else Path(args.png)
        try:
            path = run_report(store, args.month, out_md, out_png, _today(args))
        except (FileNotFoundError, ValueError) as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
        print(f"wrote {path}")
        if out_png is not None:
            print(f"wrote {out_png}")
        return 0

    if args.command == "edit":
        done = not args.clear  # default is done unless --clear given
        try:
            dates = run_edit(store, args.habit, args.date, done)
        except (FileNotFoundError, KeyError, ValueError) as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
        state = "done" if done else "cleared"
        print(f"marked {len(dates)} day(s) {state} for habit {args.habit}")
        return 0

    parser.error("unknown command")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
