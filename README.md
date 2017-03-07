Horizon is a free EDA package. It's far from finished, but the overall 
architecture is there.

# Features for users so far
- Complete design flow from schematic entry to gerber export
- Sane library management
- Unified editor for everything from schematic symbol to board
- Netlist-aware schematic editor
- Crude obstacle-avoiding router
- Lag- and glitch-free rendering
- Undo/redo
- Copy/paste for some objects
- Builds and runs on Linux and Windows

Screenshots are over [there](screenshots/)

# Features for developers
- Written in modern C++, legacy-free codebase!
- Uses JSON as on-disk format
- Uses Gtkmm3 for GUI
- OpenGL 3 accelerated rendering
- Everything is referenced by UUIDs

# Quickstart using the project manager
## Windows
Download binaries from [there](http://0x83.eu/horizon/), extract the
zip to a directory and open a command window in that directory.

### Set up the pool
Clone or download [the pool](https://github.com/carrotIndustries/horizon-pool/).
If you chose to download the pool, extract the zip somewhere. In the
command window, you opened in the previous step, type:
```
set HORIZON_POOL=<DIRECTORY WITH THE pool.json IN IT>
```
and run
```
horizon-pool-update
```
If you see `created db from schema` everything is alright.

### Launch the project manager
Double-click `horizon-prj-mgr.exe` in the directory that you opened
the command window in. You should now see a window titled "Horizon
project manager". Click on the application icon in the top left corner,
and add the pool you've set up in the preferences dialog. Now, you're
ready to create a project and use the interactive manipulator.


## Linux
Look in your distributions documentation, which packages to install to
satisfy the dependencies listed below and build horizon.

### Set up the pool
Clone or download [the pool](https://github.com/carrotIndustries/horizon-pool/).
If you chose to download the pool, extract the zip somewhere.
In the directory where you've built horizon, run
```
HORIZON_POOL=<DIRECTORY WITH THE pool.json IN IT> ./horizon-pool-update
```
If you see `created db from schema` everything is alright.

### Launch the project manager
Run
```
./horizon-prj-mgr
````
and follow the instructions from the "Windows" section above.



# How do I...
## build all this
It's as easy as `make`.

For building on Windows, use see [this guide](build_win32.md)

Dependencies:

- Gtkmm3
- libepoxy
- cairomm-pdf
- librsvg
- util-linux (on linux only)
- yaml-cpp
- sqlite
- uuid
- boost
- zeromq

On Ubuntu run:
```
sudo apt install libyaml-cpp-dev libsqlite3-dev util-linux librsvg2-dev \
    libcairomm-1.0-dev libepoxy-dev libgtkmm-3.0-dev uuid-dev libboost-dev \
    libzmq5 libzmq3-dev
```
 
## run
`HORIZON_POOL` needs to be set appropriately for all commands below
### horizon-imp
Symbol mode:  
`horizon-imp -y <symbol file>`

Schematic mode:  
`horizon-imp -c <schematic file> <block file> <constraints file>`

Padstack mode:  
`horizon-imp -a <padstack file>`

Package mode:  
`horizon-imp -k <package file>`

Board mode:  
`horizon-imp -b <board file> <block file> <constraints file> <via directory>`

### horizon-pool
Most of the -edit and -create commands will spawn $EDITOR with the file to be 
edited serialized as YAML.

```
horizon-pool create-unit <unit file>
horizon-pool edit-unit <unit file>
horizon-pool create-symbol <symbol file> <unit file>

horizon-pool create-entity <entity file> [<unit file> ...]
horizon-pool edit-entity <entity file>

#these have a GUI!
horizon-pool create-part <part file>
horizon-pool edit-part <part file>

horizon-pool create-package <package file>
horizon-pool create-padstack <padstack file>
```

Remember to run `horizon-pool-update` after creating things

### horizon-pool-update
```
horizon-pool-update
```

Recreates the pool's SQLite database.

### horizon-pool-update-parametric
```
horizon-pool-update-parametric
```

Recreates the pool's parametric SQLite database.

### horizon-prj
Use these to create empty blocks, schematics, etc.
```
horizon-prj create-block <block filename>

horizon-prj create-constraints <constraints filename>

horizon-prj create-schematic <schematic filename> <block filename> <constraints filename>

horizon-prj create-board <schematic filename> <block filename> <constraints filename>
```


## get started
After having built horizon, you should setup a pool somewhere (e.g. 
clone https://github.com/carrotIndustries/horizon-pool) and make the `HORIZON_POOL` environment variable point to it. Then run `horizon-pool-update` and `horizon-pool-update-parametric` to update both databases. Since the pool is pretty empty, there's no sensible example project right now.

Create block, constraints, schematic and board:
```
horizon-prj create-block block.json
horizon-prj create-constraints constr.json
horizon-prj create-schematic sch.json block.json constr.json
horizon-prj create-board board.json block.json constr.json
#create empty via directory
mkdir vias 

#to edit the schematic
horizon-imp -c sch.json block.json constr.json

#to edit the board schematic
horizon-imp -b board.json block.json constr.json vias


```

## use the interactive manipulator
To see the available key bindings, hit "?" when no tool is active. 
"Space" brings up a popover showing tools that can be activated. So 
far, there is no documentation on the tools itself, good luck!

# Theory of operation
run `doxygen doxygen.config` to generate HTML documentation

## Pool
[Diagram of the pool](doc/pool.pdf)

To explain horizon's operation it's best to start with the library 
structure: Actually, there are no "libraries" in horizon. Instead, all 
library data is stored in the central pool.

At the very of bottom of the Pool, there's the unit. A unit 
represents a single gate, e.g. a single amplifier of a dual operation 
amplifier with its pins.

The next level is the entity. An entity contains one or more gates that 
reference units. The entity "Quad NAND gate" thus contains four gates 
referencing the "NAND gate" unit and one gate referencing the "power" 
unit. Entities and units are generic, so the "Two-terminal resistor" 
entity can be used for any resistor of any value and package.

To represent units on the schematic, there are symbols. Contrary to 
other EDA packages, a symbol isn't the "source of truth", instead it 
references a unit from which it gathers the pin names.

Of course there are packages as well: Instead of defining each pad on 
its own, one creates a padstack describing the pads shape. This 
simplifies consistent packages and enables arbitrary pad shapes.

Packages and entities are linked in parts. Apart from the references to 
the entity and the package, a part maps the entities' pins to pads and 
contains information such as manufacturers part number and value. To 
simplify creation of similar parts, parts can inherit from another 
part. A part is supposed to be something you can actually order for 
easing the CAD-to-manufacturing transition. Furthermore, parts may 
contain parametric data where it's appropriate for easier lookup.

Parts, packages and entities can be tagged as the pool has no 
particular hierarchy.

Each of the objects described above lives in its own JSON file and is 
identified by a UUID. A SQLite database is generated from the pool to 
simplify lookup.

The pool is supposed to be global and collaborative.

## Interactive manipulator
Horizon's primary interface is the so-called "Interactive manipulator" 
(imp). It's the unified editor for symbols, schematics, padstacks, 
packages and boards. 

### Canvas
The canvas renders objects such as symbols, packages or tracks. The 
output of the rendering are line segments and triangles that get 
uploaded to the GPU for drawing. For rendering to to non-OpenGL 
targets, the canvas provides hooks to get more information on what's 
being rendered. So far, this is used by the gerber exporter and the 
obstacle-avoiding router.

### Core
Since some documents such as symbols and schematics contain the same 
type of object (e.g. texts) and schematic and netlist need to be 
modified in-sync, some encapsulation has to take place. The core can be 
considered the glue between the document, the canvas and the tools.

### Tools
For each action the user can do, there's a tool. Once started, a tool 
receives keyboard and mouse input and modifies the document 
accordingly by means of the core. When needed, a tool may bring up additional dialogs for 
requesting information from the user. 

### Property editor
Low-complexity adjustments such as line width don't warrant their own 
tool, that's why the the core provides a property interface. The 
property editor's widgets are automatically generated from the object's 
description.

# Included third-party software

- https://github.com/nlohmann/json/
- http://www.angusj.com/delphi/clipper.php
- https://github.com/ivanfratric/polypartition/
- https://qcad.org/en/what-is-dxflib
