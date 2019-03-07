#ifndef NETLIST_WALKER_H
#define NETLIST_WALKER_H
#include "vpr_types.h"

class NetlistVisitor;

class NetlistWalker {
  public:
    NetlistWalker(NetlistVisitor& netlist_visitor)
        : visitor_(netlist_visitor) {}

    void walk();

  private:
    void walk_atoms(const t_pb* pb);

  private:
    NetlistVisitor& visitor_;
};

class NetlistVisitor {
  public:
    virtual ~NetlistVisitor() = default;
    void start() { start_impl(); }
    void visit_top(const char* top_level_name) { visit_top_impl(top_level_name); }
    void visit_clb(const t_pb* clb) { visit_clb_impl(clb); }
    void visit_atom(const t_pb* atom) { visit_atom_impl(atom); }
    void finish() { finish_impl(); }

  protected:
    //All implementation methods are no-ops in this base class
    virtual void start_impl();
    virtual void visit_top_impl(const char* top_level_name);
    virtual void visit_clb_impl(const t_pb* clb);
    virtual void visit_atom_impl(const t_pb* atom);
    virtual void finish_impl();
};
#endif
