#ifndef ICE40_HLC_H
#define ICE40_HLC_H

#include "netlist_walker.h"

class ICE40HLCWriterVisitor : public NetlistVisitor {
    public:
        ICE40HLCWriterVisitor(std::ostream& f);

    private:
        void visit_top_impl(const char* top_level_name) override;
        void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) override;
        void visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb,
            const t_pb_graph_node* pb_graph_node) override;
        void finish_impl() override;

        std::ostream& os_;
};

#endif
