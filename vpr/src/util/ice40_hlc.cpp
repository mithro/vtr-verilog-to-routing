#include <iostream>

#include "ice40_hlc.h"

ICE40HLCWriterVisitor::ICE40HLCWriterVisitor(std::ostream& os)
        : os_(os) {
}

void ICE40HLCWriterVisitor::visit_top_impl(const char* top_level_name) {
    (void)top_level_name;
}

void ICE40HLCWriterVisitor::visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) {
    (void)blk_id;
    (void)clb;
}

void ICE40HLCWriterVisitor::visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb,
    const t_pb_graph_node* pb_graph_node) {
    (void)top_pb_route;
    (void)pb;
    (void)pb_graph_node;
}

void ICE40HLCWriterVisitor::finish_impl() {
}
