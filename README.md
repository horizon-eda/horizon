# Horizon EDA

Horizon EDA is an **Electronic Design Automation** package supporting an integrated end-to-end workflow for printed circuit board design including parts management and schematic entry. 

See [the docs](https://docs.horizon-eda.org/en/latest/feature-overview.html) for an overview
of horizon's top features.

Wanna chat about the project? Join #horizon on freenode

![collage](https://horizon-eda.org/screenshots/collage.png)

# Features for users so far
- Complete design flow from schematic entry to gerber export
- Sane library management
- Unified editor for everything from symbol to board
- Netlist-aware schematic editor
- KiCad's awesome interactive router
- Lag- and glitch-free rendering
- Rule-based DRC
- Undo/redo
- Copy/paste for some objects
- Builds and runs on Linux and Windows

# Features for developers
- Written in modern C++, legacy-free codebase!
- Uses JSON as on-disk format
- Uses Gtkmm3 for GUI
- OpenGL 3 accelerated rendering
- Everything is referenced by UUIDs

# Getting Started
See the [the docs](https://docs.horizon-eda.org/en/latest/installation.html)

# Included third-party software

| Directory in `3rd_party` | Project                | Version                                  | URL                                                             | License      |
|--------------------------|------------------------|------------------------------------------|-----------------------------------------------------------------|--------------|
| nlohmann                 | JSON for Modern C++    | 3.9.1                                    | https://github.com/nlohmann/json/                               | MIT          |
| clipper                  | Clipper                | 6.4.2                                    | http://www.angusj.com/delphi/clipper.php                        | Boost        |
| polypartition            | polypartition          | 7bdffb428b2b19ad1c43aa44c714dcc104177e84 | https://github.com/ivanfratric/polypartition/                   | MIT          |
| poly2tri                 | poly2tri               | TBD                                      | https://github.com/jhasse/poly2tri                              | 3-Clause BSD |
| dxflib                   | dxflib                 | TBD                                      | https://qcad.org/en/90-dxflib                                   | GPLv2        |
| alphanum                 | The Alphanum Algorithm | 1.3                                      | http://www.davekoelle.com/alphanum.html                         | MIT          |
| delaunator               | Delaunator C++         | TBD                                      | https://github.com/abellgithub/delaunator-cpp                   | MIT          |
| footag                   | footag                 | 99116328abe8f53e71831b446d35e93ee7128ef3 | https://github.com/endofexclusive/footag                        | GPLv3        |
| libzippp                 | libzip++               | TBD                                      | https://github.com/leezu/libzippp                               | ISC          |
| router                   | KiCad router           | 5.1.6                                    | https://gitlab.com/kicad/code/kicad/-/tree/master/pcbnew/router | GPLv3        |
| sexpr                    | s-expression parser    | TBD                                      | https://gitlab.com/kicad/code/kicad/-/tree/master/libs/sexpr    | GPLv3        |


- https://github.com/russdill/pybis
