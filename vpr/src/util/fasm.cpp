#include <cstring>
#include <string>
#include <vector>
#include <iostream>

#include "vtr_assert.h"

#include "globals.h"

#include "fasm.h"

// join a vector of elements by a delimiter object.  ostream<< must be defined
// for both class S and T and an ostream, as it is e.g. in the case of strings
// and character arrays
template<class S, class T>
std::string join(std::vector<T>& elems, S& delim) {
    std::stringstream ss;
    typename std::vector<T>::iterator e = elems.begin();
	if (e == elems.end())
		return "";
    ss << *e++;
    for (; e != elems.end(); ++e) {
        ss << delim << *e;
    }
    return ss.str();
}


void fasm_assert(const t_pb_type *pb, t_fasm_type type) {
}

t_fasm_type get_fasm_type(const t_pb_type *pb) {
	VTR_ASSERT(pb->name != NULL);

	std::string pb_name = std::string(pb->name);
	return _get_fasm_type(pb_name);
}

t_fasm_type _get_fasm_type(std::string pb_name) {
	VTR_ASSERT(pb_name.size() > 0);
	// 0123456
	// XXX_YY-nnnn = 3+1+2+1
	//VTR_ASSERT(pb_name.size() > 7);
	//VTR_ASSERT(pb_name[3] == '_');
	//VTR_ASSERT(pb_name[6] == '-');
	if(pb_name.size() < 7 || pb_name[3] != '_' || pb_name[6] != '-') {
		t_fasm_type ftype;
		ftype.type = FASM_NULL_TYPE;
		ftype.type_raw = 0;
		return ftype;
	}

	std::string fasm_ptype_str = pb_name.substr(0,3);
	std::string fasm_stype_str = pb_name.substr(4,2);

	t_fasm_type ftype;
	ftype.type = FASM_NULL_TYPE;
	ftype.type_raw = 0;

	for (unsigned i = 0; i < fasm_blk_typename.size(); i++) {
		if (strcmp(fasm_ptype_str.c_str(), fasm_typename[i]) == 0) {
			ftype.type = FASM_TYPES[i];
			break;
		}
	}
	VTR_ASSERT(ftype.type != FASM_NULL_TYPE);
	switch (ftype.type) {
	case FASM_BEL_TYPE:
		for (unsigned i = 0; i < fasm_bel_typename.size(); i++) {
			if (strcmp(fasm_stype_str.c_str(), fasm_bel_typename[i]) == 0) {
				ftype.type_bel = FASM_BEL_TYPES[i];
				break;
			}
		}
		break;
	case FASM_BLK_TYPE:
		if (strcmp(fasm_stype_str.c_str(), "MB") == 0) {
			ftype.type_blk = FASM_BLK_MB_TYPE;
		} else if (strcmp(fasm_stype_str.c_str(), "IG") == 0) {
			ftype.type_blk = FASM_BLK_IG_TYPE;
		} else {
			ftype.type_blk = FASM_BLK_XX_TYPE;
		}
		break;
	case FASM_PAD_TYPE:
		for (unsigned i = 0; i < fasm_pad_typename.size(); i++) {
			if (strcmp(fasm_stype_str.c_str(), fasm_pad_typename[i]) == 0) {
				ftype.type_pad = FASM_PAD_TYPES[i];
				break;
			}
		}
		break;
	default:
		VTR_ASSERT(false);
	}
	VTR_ASSERT(ftype.type_raw != 0);

	return ftype;
}

std::string get_fasm_name(const t_pb_type *pb) {
	if (pb->name == nullptr) {
		return "unknown";
	}

	std::string s(pb->name);
	auto pos = s.find('-');
	if (pos == std::string::npos) {
		std::string s("NO UNDERSCORE");
		s += pb->name;
		return s;
	}
	return s.substr(pos+1);
}

std::string get_fasm_fullname(const t_pb *pb) {
	const t_pb* curr = pb;
	std::vector<std::string> v;
	for (; curr != nullptr; curr = curr->parent_pb) {
		t_fasm_type ftype = get_fasm_type(curr->pb_graph_node->pb_type);

		if (ftype.type == FASM_BLK_TYPE) {
			switch(ftype.type_blk) {
			case FASM_BLK_IG_TYPE:
			case FASM_BLK_MB_TYPE:
				continue;
			default:
				;
			}
		}
		v.insert(v.begin(), get_fasm_name(curr->pb_graph_node->pb_type));
	}

	return join(v, ".");
}

std::string get_fasm_fullname(const t_pb_graph_node *pb_graph_node) {
	const t_pb_graph_node* curr = pb_graph_node;
	std::vector<std::string> v;
	for (; curr != nullptr; curr = curr->parent_pb_graph_node) {
		t_fasm_type ftype = get_fasm_type(curr->pb_type);
		if (ftype.type == FASM_BLK_TYPE) {
			switch(ftype.type_blk) {
			case FASM_BLK_IG_TYPE:
			case FASM_BLK_MB_TYPE:
				continue;
			default:
				;
			}
		}
		v.insert(v.begin(), get_fasm_name(curr->pb_type));
	}

	return join(v, ".");
}


FASMWriterVisitor::FASMWriterVisitor(std::ostream& os)
		: os_(std::cout)
{
}

void FASMWriterVisitor::visit_top_impl(const char* top_level_name) {
	os_ << "visit_top_impl " << top_level_name << std::endl;
    //top_module_name_ = top_level_name;
	//fprintf(fpout, "# %s architecture_id=\"%s\" atom_netlist_id=\"%s\">\n", out_fname, architecture_id.c_str(), atom_ctx.nlist.netlist_id().c_str());
}

void FASMWriterVisitor::visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) {
	auto& cluster_ctx = g_vpr_ctx.clustering();
	auto& place_ctx = g_vpr_ctx.placement();

	os_ << "visit_clb_impl " << clb->name << std::endl;
	VTR_ASSERT(clb->pb_graph_node != nullptr);
	VTR_ASSERT(clb->pb_graph_node->pb_type);
	const t_pb_type* t = clb->pb_graph_node->pb_type;

	// name
	os_ << "visit_clb_impl       name ";
	if (t->name == nullptr) {
		os_<< "null";
	} else {
		os_ << t->name;
	}
	os_ << std::endl;

	// blif_model
	os_ << "visit_clb_impl blif_model ";
	if (t->blif_model == nullptr) {
		os_ << "null";
	} else {
		os_ << t->blif_model;
	}
	os_ << std::endl;

	// class
	os_ << "visit_clb_impl      class " << TYPE_CLASS_STRING[t->class_type] << std::endl;
	os_ << "visit_clb_impl  pos x y z " << place_ctx.block_locs[blk_id].x << "," << place_ctx.block_locs[blk_id].y << std::endl;
	os_.flush();
}


void FASMWriterVisitor::visit_atom_impl(const t_pb* atom) {
	os_ << "---visit_atom_impl ";
	if (atom->name != nullptr) {
		os_ << atom->name;
	} else {
		os_ << "null";
	}
	os_ << std::endl;

	os_ << "---visit_atom_impl fasm_fullname " << get_fasm_fullname(atom) << std::endl;

//	if (atom->pb_graph_node->pb_type->num_modes == 0) {
//	    const t_model* model = atom_ctx.nlist.block_model(atom_ctx.lookup.pb_atom(atom));
//		os_ << "---visit_atom_impl num_modes " << std::endl;
//	}
//	auto atom_blk = atom_ctx.nlist.find_block(atom->name);
//	VTR_ASSERT(atom_blk);
//	for (auto& attr : atom_ctx.nlist.block_attrs(atom_ctx.lookup(atom))) {
//		os_ << "---visit_atom_impl attribute " << attr.first.c_str() << " " << attr.second << std::endl;
//	}

/*
        vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                    "Primitive '%s' not recognized by netlist writer\n", model->name);
*/
}

void FASMWriterVisitor::visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb, const t_pb_graph_node* pb_graph_node) {
	// Is the node used at all?
	bool has_atom_net = false;
	const t_pb_graph_pin* pin = nullptr;
	for(int port_index = 0; port_index < pb_graph_node->num_output_ports; ++port_index) {
		for(int pin_index = 0; pin_index < pb_graph_node->num_output_pins[port_index]; ++pin_index) {
			pin = &pb_graph_node->output_pins[port_index][pin_index];
			if (top_pb_route[pin->pin_count_in_cluster].atom_net_id != AtomNetId::INVALID()) {
				has_atom_net = true;
				goto found_atom_net;
			}
		}
	}
found_atom_net:

	VTR_ASSERT(!has_atom_net || pin != nullptr);

	os_.width(40);
	os_ << std::left << get_fasm_fullname(pb_graph_node) << " ";
	if (has_atom_net) {
		os_ << pin->port->name;
		if (pin->port->num_pins > 1) {
			os_ << "[" << pin->pin_number << "]";
		}
	} else {
		os_ << "OPEN";
	}
	os_ << " # ";

	if (pb_graph_node->pb_type->name != nullptr) {
		os_ << pb_graph_node->pb_type->name;
	} else {
		os_ << "null";
	}

	t_fasm_type ftype = get_fasm_type(pb_graph_node->pb_type);
	if (ftype.type == FASM_BEL_TYPE) {
		os_ << " bel";

		switch(ftype.type_bel) {
		case FASM_BEL_XR_TYPE:
			os_ << " Routing";
			break;
		default:
			;
		}
	} else {
		os_ << " non-bel";
	}
	os_ << std::endl;
}

void FASMWriterVisitor::finish_impl() {
}
