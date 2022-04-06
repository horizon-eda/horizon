import gi
gi.require_version('Rsvg', '2.0')
from gi.repository import Rsvg

import sys
import json

filename = sys.argv[1]
subs = sys.argv[2:]
h = Rsvg.Handle.new_from_file(filename)
dims = h.get_intrinsic_dimensions()
viewport = Rsvg.Rectangle()
viewport.width = dims.out_width.length
viewport.height = dims.out_height.length
o = {}
for sub in subs :
    x = h.get_geometry_for_layer("#"+sub, viewport)
    o["#"+sub] = {"x":x.out_ink_rect.x, "y":x.out_ink_rect.y, "width":x.out_ink_rect.width, "height": x.out_ink_rect.height}
with open(filename+".subs", "w") as fi:
    json.dump(o, fi, sort_keys=True, indent=4)
