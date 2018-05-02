#ifndef ICE40_HLC_H
#define ICE40_HLC_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "netlist_walker.h"

class ICE40HLCWriterVisitor : public NetlistVisitor {
    public:
        enum class clb_type {
            UNKNOWN,
            PLB,
            PIO
        };

        struct link {
            const t_pb_graph_pin *source_;
            const t_pb_graph_pin *sink_;
            const t_pb *pb_;
        };

    public:
        ICE40HLCWriterVisitor(std::ostream& f);

    private:
        void visit_top_impl(const char* top_level_name) override;
        void visit_atom_impl(const t_pb* atom) override;
        void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) override;
        void visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb,
            const t_pb_graph_node* pb_graph_node) override;
        void finish_impl() override;

    private:
        void process_ports(const t_pb_route *top_pb_route, const t_pb* pb, const int num_ports,
            const int *num_pins, const t_pb_graph_pin *const *pins);
        void process_route(const t_pb_route *top_pb_route, const t_pb_route *pb_route,
            const t_pb_graph_pin *pin, const t_pb* pb);
        std::list<const t_pb_graph_pin*> collect_chain(const t_pb_graph_pin *tip, bool up);
        void close_tile();

    private:
        std::ostream& os_;

        const t_pb* cur_clb_;
        clb_type cur_clb_type_;

        /// A list of source to sink links
        std::vector<link> links_;

        /// A dictionary of lines to include for the cell descriptions
        std::map<std::string, std::vector<std::string>> elements_;

        /// The text to prefix the current PLB elements with
        std::string element_prefix_;
};

#endif
