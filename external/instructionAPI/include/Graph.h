/*
 * Copyright (c) 1996-2009 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

// Graph class


#if !defined(GRAPH_H)
#define GRAPH_H

#include "dyn_detail/boost/shared_ptr.hpp"
#include <set>
#include <list>
#include <queue>

#include "Annotatable.h"
#include "Node.h"

namespace Dyninst {
class Edge;
class Graph;
class Node;
class NodeIterator;
class EdgeIterator;
    
class Graph : public AnnotatableSparse {
    friend class Edge;
    friend class Node;
    friend class Creator;
    friend class Iterator;
    
 protected:

    typedef dyn_detail::boost::shared_ptr<Node> NodePtr;
    typedef dyn_detail::boost::shared_ptr<Edge> EdgePtr;

    typedef std::set<NodePtr> NodeSet;
    typedef std::map<Address, NodeSet> NodeMap;

 public:    
    typedef dyn_detail::boost::shared_ptr<Graph> Ptr;

    // Interface class for predicate-based searches. Users
    // can inherit this class to specify the functor to use
    // as a predicate...
    class NodePredicate {

    public:
        typedef dyn_detail::boost::shared_ptr<NodePredicate> Ptr;
        virtual ~NodePredicate() {};
        virtual bool predicate(const NodePtr &node) = 0;
        static Ptr getPtr(NodePredicate *p) { 
            return Ptr(p);
        }
    };

    typedef bool (*NodePredicateFunc)(const NodePtr &node, void *user_arg);
    
    // If you want to traverse the graph start here.
    virtual void entryNodes(NodeIterator &begin, NodeIterator &end);

    // If you want to traverse the graph backwards start here.
    virtual void exitNodes(NodeIterator &begin, NodeIterator &end);
    
    // Get all nodes in the graph
    virtual void allNodes(NodeIterator &begin, NodeIterator &end);

    // Get all nodes with a provided address
    virtual bool find(Address addr, NodeIterator &begin, NodeIterator &end);

    // Get all nodes that satisfy the provided predicate
    virtual bool find(NodePredicate::Ptr, NodeIterator &begin, NodeIterator &end);
    virtual bool find(NodePredicateFunc, void *user_arg, NodeIterator &begin, NodeIterator &end);

    bool printDOT(const std::string fileName);

    virtual ~Graph() {};
    
    // We create an empty graph and then add nodes and edges.
    static Ptr createGraph();
    
    // We effectively build the graph by specifying all edges,
    // since it is meaningless to have a disconnected node. 
    void insertPair(NodePtr source, NodePtr target);
    
    virtual void insertEntryNode(NodePtr entry);
    virtual void insertExitNode(NodePtr exit);
    

    void addNode(NodePtr node);

    virtual void removeAnnotation() {};

 protected:
     
    static const Address INITIAL_ADDR;
    
    // Create graph, add nodes.
    Graph();
    
    // We also need to point to all Nodes to keep them alive; we can't 
    // pervasively use shared_ptr within the graph because we're likely
    // to have cycles.
    NodeSet nodes_;
    
    NodeMap nodesByAddr_;

    // May be overridden by children; don't assume it exists.
    // Arguably should be removed entirely.
    NodeSet entryNodes_;

    // See the above ;)
    NodeSet exitNodes_;
};

}
#endif

