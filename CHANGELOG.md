# Version 1.1.1

## Bugfixes

 - Makefile: don't build during make install ([e991aff](https://github.com/horizon-eda/horizon/commit/e991aff73919b6b0d05ea22ed7ecc2c61d8a0c0b))
 - win32: regenerate gdk pixbuf loader cache during build ([b7a128b](https://github.com/horizon-eda/horizon/commit/b7a128bed12da74accec4231baf01caa333323fe))

# Version 1.1.0

## Added features

 - Pick&place export ([bdb0af8](https://github.com/horizon-eda/horizon/commit/bdb0af86c26d50d027a2013cf52e1eaf56543af4))
 - Support replacing project metadata variables on board ([c1293e2](https://github.com/horizon-eda/horizon/commit/c1293e25a04d62c7e7a520c8b3ec3f944e6664c5))
 - Airwire filter ([145bcb1](https://github.com/horizon-eda/horizon/commit/145bcb1b9b54efed4aee7cb50b62fbf6724bf8a2))
 - Support for touchscreen pan and zoom gestures ([38ff613](https://github.com/horizon-eda/horizon/commit/38ff6139d6e8ceba7ccc2bdf234168984fc5a528), [ca1b832](https://github.com/horizon-eda/horizon/commit/ca1b8321cb7cce845eaabec28ae467e59f8c9aa9))
 - Support for "dot not populate" components ([8cf32eb](https://github.com/horizon-eda/horizon/commit/8cf32ebdecc5f6bd73578b1b5ff07c331336bec9))
 - Action for selecting all vertices of a polygon ([5d2571f](https://github.com/horizon-eda/horizon/commit/5d2571fd76ca4eb33372810dfe4897aabe16cf5a))
 - Panelisation ([188802f](https://github.com/horizon-eda/horizon/commit/188802f1e94ed23777f89c778d422b2ec6fa461f))
 - Outline layer in packages ([03f29b5](https://github.com/horizon-eda/horizon/commit/03f29b5701f9f915d777a78eb0ad74d17e875bad))
 - Automatic update of pool cache status and hint when placing out of date part ([655c084](https://github.com/horizon-eda/horizon/commit/655c08476cd14726f6d2433938d1ae9b30cd8801))
 - Recursive `git add` from git tab in pool manager ([ea5bcb4](https://github.com/horizon-eda/horizon/commit/ea5bcb420ab32d6937b76cdcf378dcc296ddcbd1))
 - Tool for resizing symbols ([21763df](https://github.com/horizon-eda/horizon/commit/21763df7d0d46a4a55b7f1a154684fe489cd5371))
 - Tool for generating silkscreen ([c314592](https://github.com/horizon-eda/horizon/commit/c314592ec88b757f0355bd6fd95ea81122ff252c))
 - Run board checks from python module ([e47e579](https://github.com/horizon-eda/horizon/commit/e47e5798957282490aa0af93cb8818a45f2a0f60))
 - Tool for rounding off polygon vertices ([b028dbe](https://github.com/horizon-eda/horizon/commit/b028dbe8c297853873521308a8d801da526106e6), [1b9a801](https://github.com/horizon-eda/horizon/commit/1b9a801a9f587501ce96cfc4de972d778e97a09e))
 - Pool update from python module ([7cd5cc2](https://github.com/horizon-eda/horizon/commit/7cd5cc2e00b78e85a29ffd7f88065ac81ce01c84))
 - Package python module as docker image ([3b047c3](https://github.com/horizon-eda/horizon/commit/3b047c3a61dfe3daf69b0612261f893c5eb384a9))
 - Export 3D rendering from python module ([619b74b](https://github.com/horizon-eda/horizon/commit/619b74b6fe1abd6f7c7c9bc835cbb8e90b3e7a1c))

## Enhancements

 - Show list of unplaced symbols in schematic ([c237eeb](https://github.com/horizon-eda/horizon/commit/c237eeb2a7f53a69f2dbc9237709b033cc6a36b6))
 - Show list of unplaced packages in board ([fabbd65](https://github.com/horizon-eda/horizon/commit/fabbd65657a2b10836c62180874695b061d70094))
 - Copy placement tool copies silkscreen ([55a3226](https://github.com/horizon-eda/horizon/commit/55a322619d57340d054a153a277df05dcf4982f2))
 - Make pool download cancellable and show download progress ([fd943ae](https://github.com/horizon-eda/horizon/commit/fd943aeddb34bbaf8902ac0a4c70b2583ce16c3e))
 - Save board display options ([fd943ae](https://github.com/horizon-eda/horizon/commit/fd943aeddb34bbaf8902ac0a4c70b2583ce16c3e))
 - Increase 3D model offset range to ±1000 mm ([fd943ae](https://github.com/horizon-eda/horizon/commit/fd943aeddb34bbaf8902ac0a4c70b2583ce16c3e))
 - Edit via tool can edit multiple vias at once ([fd943ae](https://github.com/horizon-eda/horizon/commit/fd943aeddb34bbaf8902ac0a4c70b2583ce16c3e))
 - Move project metadata such as title and author to netlist rather than having them in schematic and project file ([ef6f647](https://github.com/horizon-eda/horizon/commit/ef6f647282f759d11ee66ed86f1e103ff3fd3e2b))
 - Show name of current document in interactive manipulator window title for pool items ([72e4eec](https://github.com/horizon-eda/horizon/commit/72e4eeca2c5374e440d82a492c573a5b3e09aaed))
 - Show project title in window title of project manager, schematic and board interactive manipulator ([71e69cc](https://github.com/horizon-eda/horizon/commit/71e69ccf0713ec92504f0a93d7241b08eabf9940))
 - Show package in component head-up display ([6214fa1](https://github.com/horizon-eda/horizon/commit/6214fa1c6d90ab35bfb7f343be0e84285fd23603))
 - Support degree sign (U+00B0) in texts ([5875829](https://github.com/horizon-eda/horizon/commit/587582958876ef160f6866aaac04bc515a1f073d))
 - Support plus/minus sign (U+00B1) in texts ([3f90d8f](https://github.com/horizon-eda/horizon/commit/3f90d8f6503ced3f73d37c6a19b251d4c07de060))
 - Show status of selection filter, airwire filter and flipped view in status bar ([162a679](https://github.com/horizon-eda/horizon/commit/162a6793302870ae84c5f66fb1e3b6c8002a5270))
 - Make URLs in text clickable in head-up display ([6fd2652](https://github.com/horizon-eda/horizon/commit/6fd26526f170eee3f52c06a6a5e36a1d0e8eba7d))
 - Support for mirroring/rotating around cursor in move tool ([455599e](https://github.com/horizon-eda/horizon/commit/455599e7fa696cedd4bb68cb51ff448ca95ba7c2))
 - Search for MPNs and pin/pad names ([819687c](https://github.com/horizon-eda/horizon/commit/819687c46b466a3621cf6c4ffc9b7fa98ae564ee))
 - Support circular renumbering in renumber pads tool ([6626585](https://github.com/horizon-eda/horizon/commit/6626585e14c147c9897dafc7426f8a4c85b115e0))
 - Place pin tool can place all remaining pins at once ([97540ca](https://github.com/horizon-eda/horizon/commit/97540cac0ce71fd23438de45bfe9da97e4a4ff54))
 - Place pin tool shows preview of next autoplaced pin ([8e5335f](https://github.com/horizon-eda/horizon/commit/8e5335f2ea242040a13bc10795d62ba4e2a2da64))
 - Show pin bounding box in symbol interactive manipulator ([24a8f4b](https://github.com/horizon-eda/horizon/commit/24a8f4be5a340825a35d71a37609999c0cbe0828))
 - Selection filter dialog can be closed by escape key ([c2b169c](https://github.com/horizon-eda/horizon/commit/c2b169c48f7ffffb672a48e15bd7951bafbb569c))
 - Add context menu to recent pools and projects for opening in file browser and deleting ([040bdc7](https://github.com/horizon-eda/horizon/commit/040bdc7ae6354b85b69a89d26bfcbaa600ec4e1a))
 - Only enable export buttons if all filenames are provided ([eb2698b](https://github.com/horizon-eda/horizon/commit/eb2698bb55bff8605d53eeb6ea97b7e5e86e4f7f), [04de328](https://github.com/horizon-eda/horizon/commit/04de32871dfbbfd3af3f7d2185082d3d206da7f0), [b3b51a7](https://github.com/horizon-eda/horizon/commit/b3b51a7219390ef27a9994e64c4adcff418772c4), [2325742](https://github.com/horizon-eda/horizon/commit/2325742cacfbc9f392c301b5d8ac10d1eb9cc066), [9ac1c59](https://github.com/horizon-eda/horizon/commit/9ac1c59b2268888dc9dd6f4871064d87a5a7d917))
 - Copy paste for bus labels and bus rippers in a schematic ([3cc5375](https://github.com/horizon-eda/horizon/commit/3cc5375b98012d94cb99fffb15e2831b009ffa79))
 - Make lists in git tab sortable ([ed27677](https://github.com/horizon-eda/horizon/commit/ed2767709b670311697339343fdb96f0c5e26b21))
 - Courtyard generated by IPC footprint generator has `courtyard` parameter class ([fa9b793](https://github.com/horizon-eda/horizon/commit/fa9b793220ab053446fb6e27f61f2f73f3a05377))
 - Use tabular figures in STEP export window's progress view ([2a5cc95](https://github.com/horizon-eda/horizon/commit/2a5cc956f5e454e99af783c115aa8526a76c5dd0))
 - Improve typesetting in dimension inputs ([058fdb8](https://github.com/horizon-eda/horizon/commit/058fdb8d9a0305f88b22d6aa2c8e6b9c0c00656b))
 - Export STEP from python module ([ea5e8b2](https://github.com/horizon-eda/horizon/commit/ea5e8b2c780ab8e5b138efe468f2a664ffc30c64))
 - Support `file://` schema in Links ([fedc6f6](https://github.com/horizon-eda/horizon/commit/fedc6f6ef2a4c92bac5f4398ea376230f1c29e91))
 - Add "Work layer only" checkbox to selection filter dialog ([e464ec4](https://github.com/horizon-eda/horizon/commit/e464ec4b6266d2a0ff835af8db57cfd7b817a18f))

## Bugfixes

 - Fix copy placement tool for packages on the bottom side ([53f3ac4](https://github.com/horizon-eda/horizon/commit/53f3ac46b30d63e4d630c37bdfd1584bc849e7b7))
 - Properly escape project/pool title in recent chooser ([7777ada](https://github.com/horizon-eda/horizon/commit/7777adaed0987d524bba0f0371ef78e3ea2aa53f))
 - Fix crash when autoconnecting more than one pin per symbol ([e0567f2](https://github.com/horizon-eda/horizon/commit/e0567f2513244b503f7662cf57c20de9895a3a4b)
 - Reduce idle CPU usage in 3D preview ([162a679](https://github.com/horizon-eda/horizon/commit/162a6793302870ae84c5f66fb1e3b6c8002a5270))
 - Don't crash when encountering layer not found in layer display settings ([c11c6d9](https://github.com/horizon-eda/horizon/commit/c11c6d9d0e817fad71a80e01698a95aa1c0aa855))
 - Always remove autosaved files when exiting cleanly ([ba881b8](https://github.com/horizon-eda/horizon/commit/ba881b8fa0c3a4b2b319a02cbe38ea89f6fce008))
 - Avoid superfluous line breaks in head-up display ([5f90eaa](https://github.com/horizon-eda/horizon/commit/5f90eaae0ef3d31248a736a69133524822115e00))
 - Increase interactive manipulator to project manager socket timeout to 5 seconds to prevent socket breakage on slow machines ([ec08ec7](https://github.com/horizon-eda/horizon/commit/ec08ec7346c70db83221175e885202a9ed420cda))
 - Fix windows not getting raised on X11 and wayland when switching between project manager and interactive manipulator ([50841e5](https://github.com/horizon-eda/horizon/commit/50841e572f025c6263bffef30ce5750d0d52f932))
 - Make it possible to select polygon vertices by hover select ([3c86273](https://github.com/horizon-eda/horizon/commit/3c86273bfd84f717e9e68bff3d9f6fe4402fe002))
 - Don't smash silkscreen again if it's already omitted ([11a4c50](https://github.com/horizon-eda/horizon/commit/11a4c50c99bde77bb0da0bb21f5dc27686059e8a))
 - Delete smashed texts if package is gone during startup or netlist reload ([4f64739](https://github.com/horizon-eda/horizon/commit/4f64739468f4c20c1cd90e287e4245a7fd48ea8d))
 - Apply smooth zoom setting to all canvases ([025a46b](https://github.com/horizon-eda/horizon/commit/025a46bad0e92a77be3a67abfa41a8d2633da214), [1e6539a](https://github.com/horizon-eda/horizon/commit/1e6539a0202cd260f9560d75f979e892bb23cd2b))
 - Connect bipoles the right way round when placing them on a net line ([cf709a2](https://github.com/horizon-eda/horizon/commit/cf709a230279e2648b3434b5bde5e534f8c9925c))
 - Use WAL mode for pool databases to prevent crashes due to locked database ([4cfbe5c](https://github.com/horizon-eda/horizon/commit/4cfbe5c200a857571c593e94577bbd8fb9a7342b))
 - Automatically set window title for export file choosers based on action ([24281f3](https://github.com/horizon-eda/horizon/commit/24281f3b3b19965507e7ec6c3285a79f8cb3854a))
 - Copy/paste copies net lines attached to pins ([41c5d1e](https://github.com/horizon-eda/horizon/commit/41c5d1e117f017207c45ea802bdaff183fe69ce8))
 - Fix possible crash when copy/pasting diffpairs ([6534e8f](https://github.com/horizon-eda/horizon/commit/6534e8faebf38b8c5196dffbe73817b1fa879b3f))
 - Fix STEP export for too short slot holes ([9e3c594](https://github.com/horizon-eda/horizon/commit/9e3c59474e42fed87809f3e5d2d88544ef0c40d4))
 - Update property panels after undo/redo ([cb29541](https://github.com/horizon-eda/horizon/commit/cb29541418fb2546a3470b0d013729a61ff2b7df))
 - Use proper window title in open pool/project file chooser ([93998c0](https://github.com/horizon-eda/horizon/commit/93998c05983d17dfcb3ab63c3dc02c3fe9df6d05))
 - Fix selection preview in selection clarification menu on win32 ([db61b45](https://github.com/horizon-eda/horizon/commit/db61b4542b8f8abc51e29b50c0a9dac80f0b7e3d))
 - Fix focus passing from project manager to editor on win32 ([940aa17](https://github.com/horizon-eda/horizon/commit/940aa17dfa77869e5cbc37110b7302656b5f3998))

## Changed behavior

 - Deleting the last gate's symbol of a component in a schematic will automatically delete the entire component without the option to keep the component with no symbols visible ([f34e006](https://github.com/horizon-eda/horizon/commit/f34e006e7ddee06c1b4126611b9695c0d4498ef5))
 - Save interactive manipulator metadata such as layer visibility and grid spacing to a separate file such as `board.json.imp_meta` rather than to the board file itself ([bb1aa8c](https://github.com/horizon-eda/horizon/commit/bb1aa8c398f051d272c9576dbd783091e3a95a35))
 - Package interactive manipulator automatically deletes floating junctions and shows junctions only if selected ([acd5c44](https://github.com/horizon-eda/horizon/commit/acd5c4498f9c1d5ed4c16a0f43fba86d53bc87ad))
 - Actions in tool popover are activated with a single click rather than a double click ([576da12](https://github.com/horizon-eda/horizon/commit/576da12accf2176a836ed4634ae7feda8303efcd))

## Removals

 - `make all` doesn't include `horizon-pool` and `horizon-prj` ([12da19f](https://github.com/horizon-eda/horizon/commit/12da19f5e007891e2613015d35435c2df186b960))
 - Remove `crontab -e` style editors from `horizon-pool` ([2edd506](https://github.com/horizon-eda/horizon/commit/2edd5063f7ed4e474f22f99f6a579a4375757057))

## Misc

 - Refactor core ([fd943ae](https://github.com/horizon-eda/horizon/commit/fd943aeddb34bbaf8902ac0a4c70b2583ce16c3e))
 - Factor out search from core ([819687c](https://github.com/horizon-eda/horizon/commit/819687c46b466a3621cf6c4ffc9b7fa98ae564ee))
 - Make board rebuild a bit faster by storing a shallow copy (no expanded packages) of the board rather than a json serialisation for undo/redo ([0ffb118](https://github.com/horizon-eda/horizon/commit/0ffb11885a8176c1265d9cafafa3360ecb5721d4))
 - Make mesh generation for 3D preview independent of UI ([005ed87](https://github.com/horizon-eda/horizon/commit/005ed87d1fcef6bbedddf1f2cfe057642316d0c4))

# Version 1.0.0

No change log since this is the first versioned release.
