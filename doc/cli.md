# Command-line export

The `export` command allows exporting project artifacts from the command line without starting the Horizon-EDA GUI (headless export). This allows users to automate exporting or to export project from the command line as follows: 

```sh
horizon-eda export schematic project.hprj -o schematic.pdf
horizon-eda export gerber project.hprj --output-dir gerbers
horizon-eda export bom project.hprj -o bom.csv
horizon-eda export board project.hprj -o board.pdf
horizon-eda export pnp project.hprj --output-dir assembly
horizon-eda export step project.hprj -o board.step
horizon-eda export odb project.hprj --output-dir odb
```

Generally all of these exporters rely on the stored project settings.

- `schematic` exports the circuit diagram, with all sheets, including instantiated child blocks
- `gerber` exports the Gerber files
- `bom` exports the BOM (csv)
- `board` exports a board view (pdf) 
- `pnp` exports placement files (csv)
- `step` exports the board and enabled component models as a STEP assembly
- `odb` exports an ODB++ directory, ZIP archive or gzip-compressed tar archive

Run `horizon-eda export --help` to list commands or for example `horizon-eda export gerber --help` for command options

## Settings

Exports use the settings as they were set in the GUI, however an explicit output path is required (see options `-o FILE` or `--output-dir DIRECTORY`).

If multiple differing output formats are needed, it is possible to supply your own temporary settings to the command line interfaceusing the option `--settings FILE`. The provided settings file is a JSON file containing the overrides. 

For example here the BOM settings are adjusted to exclude no-populate elements, specify a specifc set of columns and set to sort by MPN:

```json
{
    "include_nopopulate": false,
    "csv_settings": {
        "columns": ["QTY", "MPN", "manufacturer", "refdes"],
        "sort_column": "MPN"
    }
}
```

These settings can then be used like this:

```sh

horizon-eda export bom project.hprj --settings bom.json -o bom.csv
```

Supported override fields:

| Export | Fields |
| --- | --- |
| Schematic PDF | `output_filename`, `min_line_width` |
| Board PDF | `output_filename`, `min_line_width`, `layers`, `mirror`, `reverse_layers`, `set_holes_size`, `holes_diameter` |
| PnP | `output_directory`, `filename_merged`, `filename_top`, `filename_bottom`, `mode`, `columns`, `include_nopopulate`, `customize`, `position_format`, `top_side`, `bottom_side`, `column_names` |
| STEP | `filename`, `prefix`, `include_3d_models`, `min_diameter` |
| ODB++ | `format`, `job_name`, `output_filename`, `output_directory` |
| BOM | `output_filename`, `include_nopopulate`, `orderable_MPNs`, `concrete_parts`, `csv_settings` |
| BOM CSV settings | `columns`, `sort_column`, `order`, `custom_column_names`, `column_names` |
| Gerber | `output_directory`, `prefix`, `drill_pth`, `drill_npth`, `drill_mode`, `zip_output`, `layers`, `blind_buried_drills_filenames` (Gerber layer keys are the numeric layer IDs used in the board JSON) |

These fields use the same units and values as the saved export settings. For example, PDF `min_line_width` is in nanometers, BOM `order` is `asc` or `desc`, and Gerber `drill_mode` is `merged` or `individual`. Each Gerber layer entry has `filename`, `enabled`, and an optional serialized `layer` ID. Blind and buried drill entries contain `span` with `start` and `end` layer IDs, plus `filename`. BOM `concrete_parts` maps source part UUIDs to replacement part UUIDs while`orderable_MPNs` maps part UUIDs to orderable MPN UUIDs.

The Gerber and STEP commands also accept `--prefix PREFIX`:

```sh
horizon-eda export gerber project.hprj --output-dir gerbers --prefix controller
```

Gerber filenames must be plain filenames within the output directory.  The setting`zip_output` adds a ZIP archive alongside the generated layers and drills.

## Board and assembly settings

These examples can be saved as JSON files and passed with `--settings FILE`. Fields not included in the file keep their saved values. Nested objects are merged, while arrays such as `columns` replace the saved array.

**Board PDF**

Save as `board.json` to set layer colors and rendering modes, with a minimum line width of 0.1 mm and a fixed hole diameter of 0.3 mm:

```json
{
    "min_line_width": 100000,
    "mirror": false,
    "reverse_layers": false,
    "set_holes_size": true,
    "holes_diameter": 300000,
    "layers": {
        "0": {
            "enabled": true,
            "mode": "fill",
            "color": {"r": 0.8, "g": 0.2, "b": 0.1}
        },
        "-100": {
            "enabled": false,
            "mode": "fill",
            "color": {"r": 0.1, "g": 0.2, "b": 0.8}
        },
        "100": {
            "enabled": true,
            "mode": "outline",
            "color": {"r": 0, "g": 0, "b": 0}
        }
    }
}
```

```sh
horizon-eda export board project.hprj -o board.pdf --settings board.json
```

The `layers` object uses the following IDs as JSON keys, for example `"0"` for top copper:

| Layer ID | Layer |
| --- | --- |
| `200` | Top notes |
| `110` | Outline notes |
| `100` | Board outline |
| `60` | Top courtyard |
| `50` | Top assembly |
| `40` | Top package |
| `30` | Top paste |
| `20` | Top silkscreen |
| `10` | Top solder mask |
| `0` | Top copper |
| `-1` | Inner copper 1 |
| `-2` | Inner copper 2 |
| `-3` | Inner copper 3 |
| `-4` | Inner copper 4 |
| `-5` | Inner copper 5 |
| `-6` | Inner copper 6 |
| `-7` | Inner copper 7 |
| `-8` | Inner copper 8 |
| `-100` | Bottom copper |
| `-110` | Bottom solder mask |
| `-120` | Bottom silkscreen |
| `-130` | Bottom paste |
| `-140` | Bottom package |
| `-150` | Bottom assembly |
| `-160` | Bottom courtyard |
| `-200` | Bottom notes |
| `1000` | User 1 |
| `1001` | User 2 |
| `1002` | User 3 |
| `1003` | User 4 |
| `1004` | User 5 |
| `1005` | User 6 |
| `1006` | User 7 |
| `1007` | User 8 |
| `10000` | Holes (PDF only) |
| `10001` | Dimensions (PDF only) |

Only include board layers that exist in the project, including its configured inner copper and user layers. The two PDF-only layers can also be configured for board PDF export.

Other layers retain their saved settings; disable them explicitly if they should not appear. When no layer settings have been saved, the CLI supplies all board layers in black.

The field `mode` accepts the values `fill` or `outline`, while color components range from 0.0 to 1.0. The boolean switches `mirror ` and `reverse_layers` accept the values `true` or `false`. The field `set_holes_size` enables the fixed hole diameter with dimensions in nanometers (so 1000000 corresponds to 1 mm). At least one layer must remain enabled.

**Pick-and-place CSV**

Save as `pnp.json` to export a single file with custom column names, side labels and coordinates in millimeters with three decimal places:

```json
{
    "mode": "merged",
    "filename_merged": "placement.csv",
    "include_nopopulate": false,
    "customize": true,
    "columns": ["refdes", "MPN", "value", "manufacturer", "package", "x", "y", "angle", "side"],
    "column_names": {
        "refdes": "Designator",
        "x": "X (mm)",
        "y": "Y (mm)",
        "angle": "Rotation",
        "side": "Layer"
    },
    "top_side": "Top",
    "bottom_side": "Bottom",
    "position_format": "%.3m"
}
```

```sh
horizon-eda export pnp project.hprj --output-dir assembly --settings pnp.json
```

The `columns` array above shows all supported columns; select and reorder them as needed. `column_names`, `top_side` and `bottom_side` accept custom text. Set `customize` to `false` to use the standard names, side labels and position formatting, or `include_nopopulate` to `true` to include unpopulated components.

For separate files with standard formatting, save this alternative as `pnp.json`:

```json
{
    "mode": "individual",
    "filename_top": "placement-top.csv",
    "filename_bottom": "placement-bottom.csv",
    "include_nopopulate": false,
    "customize": false,
    "columns": ["refdes", "x", "y", "angle", "side"]
}
```

The two `mode` values are `merged` and `individual`. Filenames must be plain filenames, with distinct names for separate side files. If no filenames have been configured, the defaults are `positions.csv`, `top.csv` and `bottom.csv`.

With `customize` enabled, possible position formats include:

| `position_format` | Coordinates |
| --- | --- |
| `%.3m` | Millimeters with three decimal places |
| `%.0u` | Micrometers without decimal places |
| `%.4i` | Inches with four decimal places |
| `%.1t` | Mils with one decimal place |

The precision can be any single digit from 0 to 9. Update custom column labels to match the selected unit.

**STEP assembly**

Save as `step.json` to include component models, prefix assembly and component names, and omit holes smaller than 0.3 mm:

```json
{
    "include_3d_models": true,
    "prefix": "controller_",
    "min_diameter": 300000
}
```

```sh
horizon-eda export step project.hprj -o board.step --settings step.json
```

Set `include_3d_models` to `false` to export only the board, or `min_diameter` to `0` to include holes of all sizes. The diameter is in nanometers. `prefix` accepts a string, including an empty string for no prefix; `--prefix PREFIX` overrides it on the command line.

Enabled component models must be available locally, including referenced files beneath `pool/3d_models`. An unreadable selected model or invalid board outline fails the export.

**ODB++ job**

Save one of these examples as `odb.json`. A directory job:

```json
{
    "format": "directory",
    "job_name": "controller"
}
```

A ZIP archive:

```json
{
    "format": "zip",
    "job_name": "controller",
    "output_filename": "controller.zip"
}
```

A gzip-compressed tar archive:

```json
{
    "format": "tgz",
    "job_name": "controller",
    "output_filename": "controller.tgz"
}
```

```sh
horizon-eda export odb project.hprj --output-dir odb --settings odb.json
```

These are all three supported `format` values. The examples produce `odb/controller/`, `odb/controller.zip` or `odb/controller.tgz`, respectively. Archive format is selected by `format`, so choose a matching filename extension. Only the basename of `output_filename` is used, keeping the archive inside `--output-dir`.

Set `output_filename` to an empty string to derive the archive name from the normalized job name and format. Simply omitting the field retains any saved filename. An existing directory job with extra files is rejected even with `--overwrite`, so stale layers cannot remain in the new job.

## CI behavior

- No display server, GUI preferences, or configured external pools are required
- All referenced pool items must be present in the project's local pool or its committed cache directories
- A temporary pool index is built for each invocation, so `pool.db` need not be committed
- Project files and the source pool are not modified
- Copper planes are recalculated in memory before Gerber, board PDF, and ODB++ generation
- Loading warnings fail the export to avoid publishing incomplete data
- Output parent directories are created as needed
- Existing output files require `--overwrite` and unrelated files in the destination are retained
- Symbolic-link output files and destinations that would replace project inputs are rejected
- Artifacts are generated in a temporary directory before they are copied to their destinations
- A failure while publishing multiple files can leave a partial artifact set, so CI should only publish artifacts after a successful command
- Progress and diagnostics go to stderr, leaving stdout empty during exports
- `--quiet` suppresses progress while retaining diagnostics

Exit codes:

| Code | Meaning |
| --- | --- |
| 0 | Success |
| 1 | Project loading, missing dependencies, output conflicts, or export failure |
| 2 | Invalid arguments or export settings |


