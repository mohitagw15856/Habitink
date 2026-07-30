import datetime as dt
from pathlib import Path

from habitink import cli
from habitink.model import DataStore
from habitink.report import build_markdown, run_report

TODAY = dt.date(2026, 7, 30)


def test_build_markdown_has_summary(store):
    config = store.read_config()
    md = build_markdown(config, store, 2026, 7, TODAY, "grid.png")
    assert "# HabitInk report: July 2026" in md
    assert "Drink water" in md
    assert "![Streak grids](grid.png)" in md
    assert "| Habit |" in md
    # Habit 1 was done 26..30 July (5 days), all due (daily).
    assert "| Drink water | daily | 5 |" in md


def test_run_report_writes_md_and_png(store, tmp_path):
    out_md = tmp_path / "out" / "report.md"
    out_png = tmp_path / "out" / "grid.png"
    run_report(store, "2026-07", out_md, out_png, today=TODAY)
    assert out_md.exists()
    assert out_png.exists()
    assert out_png.stat().st_size > 0
    # PNG referenced relatively in the markdown.
    assert "grid.png" in out_md.read_text()


def test_run_report_png_optional(store, tmp_path):
    out_md = tmp_path / "report.md"
    run_report(store, "2026-07", out_md, None, today=TODAY)
    assert out_md.exists()
    assert "![" not in out_md.read_text()


def test_run_report_defaults_to_current_month(store, tmp_path):
    out_md = tmp_path / "report.md"
    run_report(store, None, out_md, None, today=TODAY)
    assert "July 2026" in out_md.read_text()


def test_cli_report(sd, tmp_path, capsys):
    out_md = tmp_path / "r.md"
    out_png = tmp_path / "r.png"
    rc = cli.main([
        "report", "--sd", str(sd), "--today", "2026-07-30",
        "--month", "2026-07", "--out", str(out_md), "--png", str(out_png),
    ])
    assert rc == 0
    assert out_md.exists() and out_png.exists()
    assert "wrote" in capsys.readouterr().out


def test_cli_report_png_none(sd, tmp_path):
    out_md = tmp_path / "r.md"
    rc = cli.main([
        "report", "--sd", str(sd), "--today", "2026-07-30",
        "--out", str(out_md), "--png", "none",
    ])
    assert rc == 0
    assert out_md.exists()


def test_cli_edit(sd):
    rc = cli.main([
        "edit", "--sd", str(sd), "--habit", "2", "--date", "2026-07-28", "--done",
    ])
    assert rc == 0
    store = DataStore.locate(sd)
    assert store.read_log(2).is_done(dt.date(2026, 7, 28))


def test_cli_edit_clear(sd):
    cli.main(["edit", "--sd", str(sd), "--habit", "1", "--date", "2026-07-30", "--clear"])
    store = DataStore.locate(sd)
    assert not store.read_log(1).is_done(dt.date(2026, 7, 30))


def test_cli_report_missing_config(tmp_path, capsys):
    rc = cli.main(["report", "--sd", str(tmp_path), "--out", str(tmp_path / "r.md"), "--png", "none"])
    assert rc == 1
    assert "error" in capsys.readouterr().err


def test_cli_edit_unknown_habit(sd, capsys):
    rc = cli.main(["edit", "--sd", str(sd), "--habit", "nope", "--date", "2026-07-30"])
    assert rc == 1
    assert "error" in capsys.readouterr().err
