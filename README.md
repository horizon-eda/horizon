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
- https://github.com/nlohmann/json/
- http://www.angusj.com/delphi/clipper.php
- https://github.com/ivanfratric/polypartition/
- https://github.com/jhasse/poly2tri
- https://qcad.org/en/what-is-dxflib
- https://git.launchpad.net/kicad/tree/pcbnew/router
- https://github.com/russdill/pybis
