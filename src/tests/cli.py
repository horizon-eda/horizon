import csv
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import tarfile
import uuid
import zipfile


executable = str(Path(sys.argv[1]).resolve())
fixture = str(Path(sys.argv[2]).resolve())


def snapshot(directory):
    """Remember the project file contents so we can check that exporting leaves them alone"""
    # Compare contents rather than timestamps, so rewriting a file with different data is caught
    return {
        str(path.relative_to(directory)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in directory.rglob("*")
        if path.is_file()
    }


with tempfile.TemporaryDirectory(prefix="horizon-cli-test-") as temporary:
    root = Path(temporary)
    project = root / "project with spaces"
    subprocess.run([fixture, str(project)], check=True)
    # Remove display settings and use fresh config paths, so the test does not rely on the desktop session
    env = dict(os.environ)
    env.pop("DISPLAY", None)
    env.pop("WAYLAND_DISPLAY", None)
    env["XDG_CONFIG_HOME"] = str(root / "config")
    env["XDG_CACHE_HOME"] = str(root / "cache")
    before = snapshot(project)

    def run(*args, status=0):
        """Run the CLI without a display and include its output if the exit status is not what we expected"""
        result = subprocess.run(
            [executable, *map(str, args)], cwd=root, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60,
        )
        assert result.returncode == status, (args, result.returncode, result.stdout, result.stderr)
        return result

    def settings(value):
        """Save JSON overrides for the next export command"""
        path = root / "settings.json"
        path.write_text(json.dumps(value))
        return path

    # Help and argument validation should work before any project is loaded
    prj = project / "test.hprj"
    assert "horizon-eda" in run("--version").stdout
    assert "schematic" in run("export", "--help").stdout
    for command in ("schematic", "gerber", "bom", "board", "pnp", "step", "odb"):
        assert "--settings" in run("export", command, "--help").stdout
    for command in ("schematic", "board"):
        for filename in ("output.svg", "output", "output.pdf.png"):
            run("export", command, "missing.hprj", "-o", filename, status=2)
        assert ".pdf" in run("export", command, "--help").stdout
    run("export", "unknown", status=2)
    run("export", "bom", prj, status=2)
    run("export", "bom", prj, "--output-dir", "bad", status=2)
    run("export", "gerber", prj, "-o", "bad", status=2)
    run("export", "bom", prj, "-o", "a.csv", "--output", "b.csv", status=2)
    run("export", "bom", prj, "--unknown", status=2)
    run("export", "bom", "missing.hprj", "-o", "missing.csv", status=1)

    # The schematic should contain both top sheets, the child sheet and its embedded picture
    # An uppercase extension should work too, and --quiet should leave both output streams empty
    pdf = root / "artifacts/schematic.PDF"
    result = run("export", "schematic", prj, "-o", pdf, "--quiet")
    assert not result.stdout and not result.stderr, result
    data = pdf.read_bytes()
    assert data.startswith(b"%PDF-")
    assert len(re.findall(rb"/Type\s*/Page\b", data)) == 3
    assert re.search(rb"/Subtype\s*/Image\b", data)

    # Start with the saved BOM settings, which exclude R2
    # Then include it through an override and check that both resistors end up in the same row
    bom = root / "artifacts/bom.csv"
    result = run("export", "bom", prj, "--output=artifacts/bom.csv", "--quiet")
    assert not result.stdout and not result.stderr, result
    rows = list(csv.reader(bom.open(newline="")))
    assert "TEST-10K" in rows[1] and "R1" in rows[1] and "R2" not in rows[1]
    # Trying to overwrite without permission must leave the existing file alone
    old_bom = bom.read_bytes()
    run("export", "bom", prj, "-o", bom, status=1)
    assert bom.read_bytes() == old_bom
    run("export", "bom", prj, "-o", bom, "--overwrite", "--settings",
        settings({"include_nopopulate": True, "csv_settings": {"columns": ["MPN", "QTY"]}}))
    rows = list(csv.reader(bom.open(newline="")))
    assert rows[1] == ["TEST-10K", "2"], rows

    # Misspelled settings, wrong types and invalid column names should fail without producing output
    for invalid in ({"typo": True}, {"csv_settings": {"colums": []}},
                    {"include_nopopulate": "yes"}, {"csv_settings": {"columns": ["invalid"]}},
                    {"csv_settings": None}):
        run("export", "bom", prj, "-o", "invalid.csv", "--settings", settings(invalid), status=2)
        assert not (root / "invalid.csv").exists()
    invalid_json = root / "invalid.json"
    invalid_json.write_text("{")
    run("export", "bom", prj, "-o", "invalid.csv", "--settings", invalid_json, status=2)

    # Check for a complete Gerber file, filled copper, an outline and the mounting hole at 5 mm
    gerbers = root / "artifacts/gerbers"
    result = run("export", "gerber", prj, "--output-dir", gerbers, "--prefix", "ci", "--quiet")
    assert not result.stdout and not result.stderr, result
    assert (gerbers / "ci.gtl").read_text().endswith("M02*\n")
    assert "G36*" in (gerbers / "ci.gtl").read_text()
    assert "G36*" in (gerbers / "ci.gko").read_text() or "D01*" in (gerbers / "ci.gko").read_text()
    assert "X5.000Y5.000" in (gerbers / "ci.txt").read_text()
    old_gerbers = snapshot(gerbers)
    run("export", "gerber", prj, "--output-dir", gerbers, "--prefix", "ci", status=1)
    assert snapshot(gerbers) == old_gerbers
    # The ZIP should include the layer files and the separate non-plated drill file
    run("export", "gerber", prj, "--output-dir", gerbers, "--prefix", "ci", "--overwrite",
        "--settings", settings({"zip_output": True, "drill_mode": "individual"}))
    assert (gerbers / "ci-npth.txt").exists()
    with zipfile.ZipFile(gerbers / "ci.zip") as archive:
        assert "ci.gtl" in archive.namelist() and "ci-npth.txt" in archive.namelist()
    # Reject paths that escape the output directory and filenames that collide with another output
    run("export", "gerber", prj, "--output-dir", "bad-gerbers", "--prefix", "../escape", status=2)
    run("export", "gerber", prj, "--output-dir", "bad-gerbers", "--settings",
        settings({"drill_pth": ".gtl"}), status=2)
    assert not (root / "bad-gerbers").exists()

    # The board PDF should have one page and accept partial overrides of its saved layer settings
    board_pdf = root / "artifacts/board.pdf"
    result = run("export", "board", prj, "-o", board_pdf, "--quiet")
    assert not result.stdout and not result.stderr, result
    assert board_pdf.read_bytes().startswith(b"%PDF-")
    assert len(re.findall(rb"/Type\s*/Page\b", board_pdf.read_bytes())) == 1
    run("export", "board", prj, "-o", board_pdf, status=1)
    run("export", "board", prj, "-o", board_pdf, "--overwrite", "--settings",
        settings({"mirror": True, "layers": {"0": {"color": {"r": 0.25}}}}))
    run("export", "board", prj, "-o", "invalid.pdf", "--settings",
        settings({"layers": {"123456": {"enabled": True}}}), status=2)
    run("export", "board", prj, "-o", "invalid.pdf", "--settings",
        settings({"layers": {"0": {"color": {"r": 2}}}}), status=2)

    # Check the saved placement filename and coordinates first, then export both sides separately
    # Including R2 also lets us check the custom label for the bottom side
    assembly = root / "artifacts/assembly"
    result = run("export", "pnp", prj, "--output-dir", assembly, "--quiet")
    assert not result.stdout and not result.stderr, result
    rows = list(csv.reader((assembly / "saved-positions.csv").open(newline="")))
    assert len(rows) == 2 and rows[1][0] == "R1", rows
    assert rows[1][1:3] == ["10.000", "12.000"], rows
    run("export", "pnp", prj, "--output-dir", assembly, status=1)
    run("export", "pnp", prj, "--output-dir", assembly, "--settings",
        settings({"mode": "individual", "include_nopopulate": True,
                  "columns": ["refdes", "side"], "customize": True, "bottom_side": "back"}))
    assert list(csv.reader((assembly / "saved-top.csv").open(newline="")))[1] == ["R1", "top"]
    assert list(csv.reader((assembly / "saved-bottom.csv").open(newline="")))[1] == ["R2", "back"]
    run("export", "pnp", prj, "--output-dir", "invalid-pnp", "--settings",
        settings({"filename_merged": "../outside.csv"}), status=2)
    run("export", "pnp", prj, "--output-dir", "invalid-pnp", "--settings",
        settings({"mode": "individual", "filename_top": "same.csv", "filename_bottom": "same.csv"}), status=2)
    run("export", "pnp", prj, "--output-dir", "invalid-pnp", "--settings",
        settings({"customize": True, "position_format": "%s"}), status=2)

    # Check the STEP file header and that the CLI prefix overrides the saved assembly prefix
    step = root / "artifacts/board.step"
    result = run("export", "step", prj, "-o", step, "--quiet", "--prefix", "ci_")
    assert not result.stdout and not result.stderr, result
    assert step.read_text().startswith("ISO-10303-21;")
    assert "ci_PCB" in step.read_text()
    run("export", "step", prj, "-o", step, status=1)
    run("export", "step", prj, "-o", step, "--overwrite", "--settings",
        settings({"include_3d_models": False, "min_diameter": 1000000}))

    # Export the same ODB++ job as a tar archive, a ZIP and a directory
    # The matrix file should be present in each, under the saved job name
    odb = root / "artifacts/odb"
    result = run("export", "odb", prj, "--output-dir", odb, "--quiet")
    assert not result.stdout and not result.stderr, result
    with tarfile.open(odb / "assembly.tgz") as archive:
        assert "assembly/matrix/matrix" in archive.getnames()
    run("export", "odb", prj, "--output-dir", odb, status=1)
    run("export", "odb", prj, "--output-dir", odb, "--settings",
        settings({"format": "zip", "output_filename": "assembly.zip"}))
    with zipfile.ZipFile(odb / "assembly.zip") as archive:
        assert "assembly/matrix/matrix" in archive.namelist()
    run("export", "odb", prj, "--output-dir", odb, "--settings", settings({"format": "directory"}))
    assert (odb / "assembly/matrix/matrix").is_file()
    old_odb = snapshot(odb)
    run("export", "odb", prj, "--output-dir", odb, "--settings",
        settings({"format": "directory"}), status=1)
    assert snapshot(odb) == old_odb
    run("export", "odb", prj, "--output-dir", odb, "--overwrite", "--settings",
        settings({"format": "directory"}))
    # An extra file in an old ODB++ job must not be carried into the new job, even with --overwrite
    stale_file = odb / "assembly/stale-layer"
    stale_file.write_text("keep this file")
    stale_odb = snapshot(odb)
    run("export", "odb", prj, "--output-dir", odb, "--overwrite", "--settings",
        settings({"format": "directory"}), status=1)
    assert snapshot(odb) == stale_odb
    run("export", "odb", prj, "--output-dir", "invalid-odb", "--settings",
        settings({"format": "invalid"}), status=2)

    # Output paths must not replace project inputs, write into the pool or follow an output symlink
    run("export", "bom", prj, "-o", project / "top_block.json", "--overwrite", status=1)
    run("export", "bom", prj, "-o", project / "pool/new.csv", status=1)
    if os.name != "nt":
        link = root / "linked.csv"
        link.symlink_to(bom)
        run("export", "bom", prj, "-o", link, "--overwrite", status=1)
    # All exports so far should have left the project alone and needed no personal config or cache
    assert snapshot(project) == before
    assert not (root / "config").exists()
    assert not (root / "cache").exists()

    # Now deliberately change the fixture to test model loading failures
    # A missing model should fail STEP export, but exporting with models disabled should still work
    package_file = project / "pool/packages/test/package.json"
    package = json.loads(package_file.read_text())
    model_uuid = str(uuid.uuid4())
    package["models"] = {model_uuid: {"filename": "3d_models/test.step", "x": 0, "y": 0, "z": 0,
                                    "roll": 0, "pitch": 0, "yaw": 0}}
    package["default_model"] = model_uuid
    package_file.write_text(json.dumps(package))
    run("export", "step", prj, "-o", "missing-model.step", "--quiet", status=1)
    assert not (root / "missing-model.step").exists()
    run("export", "step", prj, "-o", "without-models.step", "--settings",
        settings({"include_3d_models": False}), "--quiet")
    model_file = project / "pool/3d_models/test.step"
    model_file.write_text("invalid STEP data")
    run("export", "step", prj, "-o", "invalid-model.step", "--quiet", status=1)
    assert not (root / "invalid-model.step").exists()
    # Reuse the board STEP file as a valid component model, so no external model file is needed
    model_file.write_bytes(step.read_bytes())
    with_models = snapshot(project)
    run("export", "step", prj, "-o", "with-models.step", "--quiet")
    assert "R1" in (root / "with-models.step").read_text()
    assert snapshot(project) == with_models

    # Remove the outline to check that STEP and ODB++ fail without leaving output
    # Restore it afterwards so the remaining tests still have a valid board
    board_file = project / "board.json"
    original_board = board_file.read_bytes()
    board = json.loads(original_board)
    board["polygons"] = {key: value for key, value in board["polygons"].items() if value["layer"] != 100}
    board_file.write_text(json.dumps(board))
    run("export", "step", prj, "-o", "no-outline.step", "--quiet", status=1)
    run("export", "odb", prj, "--output-dir", "no-outline-odb", "--quiet", status=1)
    assert not (root / "no-outline.step").exists()
    assert not (root / "no-outline-odb").exists()
    board_file.write_bytes(original_board)

    # Missing pictures and pool parts should fail the export rather than produce incomplete files
    for path in (project / "pictures").iterdir():
        path.unlink()
    run("export", "schematic", prj, "-o", "incomplete.pdf", "--quiet", status=1)
    assert not (root / "incomplete.pdf").exists()
    (project / "pool/parts/part.json").unlink()
    run("export", "bom", prj, "-o", "incomplete.csv", "--quiet", status=1)
    assert not (root / "incomplete.csv").exists()

print("CLI integration checks passed")
