

# Create grid
max_conn = {}

# Loop over all the coordinates and work out the number of wire<->wire
# connections between each.

# Assume that delta is only +1 in any direction
deltas = set()
for info in tileconn_json:
    deltas.add(info['grid_deltas'])

# Convert the tileconn.json into the following datastructure...
relative_to = {
    'from_tiletype': {
        ('delta_x', 'delta_y'): {
            'to_filetype_a': [('from_wire', 'to_wire'), '...'],
            'to_filetype_b': [('from_wire', 'to_wire'), '...'],
        },
    },
}

def add_relative(from_tiletype, to_tiletype, delta, wire_pairs):
    if from_tiletype not in relative_to:
        relative_to[from_tiletype] = {}

    if (from_x, from_y) not in relative_to[from_tiletype]:
        relative_to[from_tiletype] = {}

    relative_to[from_tiletype][delta][to_tiletype] = wire_pairs


for info in tileconn_json:
    from_tiletype, to_tiletype = info['tile_types']
    from_x, from_y = info['grid_deltas']

    add_relative(from_tiletype, to_tiletype, (from_x, from_y), info['wire_pairs'])
    add_relative(to_tiletype, from_tiletype, (-from_x, -from_y), [(b, a) for a, b in info['wire_pairs']])


# Loop over all the grid positions and work out how many wires are in each channel
x_channel = {}
y_channel = {}
wires = []
for (x, y) in grid.coordinates:
    from_tiletype = grid[(x,y)]

    wire_pairs_x = []
    wire_pairs_y = []

    for delta in relative_to[from_tiletype]:
        to_pos = (x, y) + delta
        to_tiletype = grid[to_pos]
        if to_tiletype not in relative_to[from_tiletype][delta]:
            continue

        # FIXME: Support non-single tile N/E/S/W deltas
        if delta.x in (1, -1):
            assert delta.y == 0
            wire_pairs_x.extend(relative_to[from_tiletype][delta][to_tiletype])
        elif delta.y in (1, -1):
            assert delta.x == 0
            wire_pairs_y.extend(relative_to[from_tiletype][delta][to_tiletype])
        else:
            assert False, "Can't use delta {} at the moment".format(delta)

    x_channels[x][y] = wire_pairs_x
    y_chhanels[y][x] = wire_pairs_y

# Allocate the channel widths
x_channel_widths = [max(len(c) for c in x_channels)]
y_channel_widths = [max(len(c) for c in y_channels)]

num_channel_nodes = sum(x_channel_widths) + sum(y_channel_widths)
num_pin_nodes = XXXX
nodes = sizeof(rr_node) * (num_channel_nodes + num_pin_nodes)

num_channel_edges = num_channel_nodes
num_pip_edges = YYYY
edges = sizeof(rr_edge) * (num_channel_edges + num_pip_edges)

# Populate the tileconn edges with shorts
for (x, y) in grid.coordinates:
    add_edge(lookup_node_id((x,y), from_name), lookup_node_id((x, y), to_name), edge_type=short)

# Populate the PIP edges



