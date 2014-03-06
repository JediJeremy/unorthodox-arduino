
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
 */

#ifndef UNORTHODOX_TREES_H
#define UNORTHODOX_TREES_H

/*
  Prefix Tree Structure:
  tree : [{node}]<*>
  node : {head}[{span}]<head.span>[{leaf}]<head.leaf>[{radix}{prefix}{catalog}]<head.radix>
  head : {card} head.size=bits[3..]  head.leaf=bit[2]  head.span=bit[1]  head.radix=bit[0]
  leaf : {card}
  span : [{char}]<head.size>
  radix : [{card}]<!head.span && head.catalog> | <head>
  prefix : [{char}]<radix>
  catalog : [{card}]<radix>

  {token} [optional]<counter>
 */
struct PrefixNodeResult {
	int token;
	int chars;
};

class PrefixTree {
public:
	/* untested 
    static PrefixNodeResult tree_select(ReadPage * page, word tree, byte * str, word bytes) {
      PrefixNodeResult r;
      // extract the tree tokens cardinal
      word tokens = Cardinal::decode_word(page,tree);
      word root = tree + Cardinal::decode_size(page,tree);
      word node = root;
      int consumed = 0;
      // if zero, then it's an empty tree, and we're done.
      if(tokens>0) {
        // now begin iterating through the tree nodes
        bool more = true;
        while(more) {
          // match the next segment
          r = node_select(page, node, str, bytes);
          // did we get a token?
          if(r.token>=0) {
            // that level suceeded. was the token a result symbol or tree index?
            if(r.token<tokens) {
              // we have got a leaf symbol. all done!
              r.chars += consumed;
              return r;
            }
            // consume the characters
            consumed += r.chars;
            bytes -= r.chars;
            str += r.chars;
            // reset the node pointer
            node = root + r.token - tokens;
            // have we consumed all our string characters?
            more = bytes > 0;
          } else {
            // the node could not match the string.
            more = false;
          }
        }
      }
      // failed
      r.token = -1; r.chars = 0; return r;
    }

    static PrefixNodeResult node_select(ReadPage * page, word node, byte * str, word bytes) {
      PrefixNodeResult r;
      r.chars = 0;
      // how many remain to be processed
      int remain = bytes; int sindex = 0;
      // get the node header
      int head_bytes = Cardinal::decode_size(page,node);
      word node_head = Cardinal::decode_word(page,node);
      // seperate out some header properties
      bool head_leaf = ( (node_head & 0x04) != 0 );
      bool head_span = ( (node_head & 0x02) != 0 );
      bool head_radix = ( (node_head & 0x01) != 0 );
      int head_count = node_head/8;
      // maintain a local index pointer into the block
      int index = head_bytes;
      // have we reached the end of the select string?
      if(remain==0) {
        // are we at a leaf node?
        if(head_leaf) {
          // we are done. consume and return the leaf card.
          r.token = Cardinal::decode_word(page, node + index);
          return r;
        } else {
          // this is not a leaf node.
          r.token = -1; return r;
        }
      }
      // skip the leaf card, if it exists
      if(head_leaf) {
        index += Cardinal::decode_size(page, node + index);
      }
      // consume the span
      int c = head_count;
      if(remain<c) {
        // we are going to run out of selection characters within the span. therefore, we fail regardless.
        r.token = -1; return r;
      } else {
        // string compare the span with next selection characters
        while(c) {
          if(page->read_byte(node + index++)==str[sindex++]) { c--; r.chars++; } else { r.token = -1; return r; }
        }
      }
      // consume the prefix block
      if(head_radix) {
        int radix;
        if(head_span) {
          // the prefix block has it's own radix byte
          radix = page->read_byte(node + index++);
        } else {
          // there was no span, so we get to use the header count field
          radix = head_count;
        }
        // consume the next string character and search the prefix block
        int order = prefix_select(page, radix, node+index, str[sindex++]);
        if(order==-1) {
          // fail. there was no prefix match.
          r.token = -1; return r;
        } else {
          // we have an order index. return the catalog entry.
          index += radix;
          r.token = page->read_word(node + index + order*2);
          r.chars++;
          return r;
        }
      }
      // none of that worked.
      r.token = -1; return r;
    }
	 */

	// seek the next character byte in an ordered prefix character table, and return it's index (or -1 if not found)
	static int prefix_select(ReadPage * page, int radix, word table, byte c) {
		// fail on empty tables
		if(radix<=0) { return -1; }
		// start a recursive approximation on the ordered list
		int i = 0; int j = radix-1; 
		while(true) {
			if(i==j) {
				// only one option left.
				if(page->read_byte(table + i)==c) {
					return i; // success!
				} else {
					return -1; // fail!
				}
			} else {
				// get the midpoint
				int m = (i+j)/2;
				// test the character
				int d = page->read_byte(table + m) - c;
				if(d==0) {
					// found it.
					return m;
				} else if(d>0) {
					// in the lower half
					j = (m>i) ? (m-1) : i;
				} else {
					// in the upper half
					i = (m<j) ? (m+1) : j;
				}
			}
		}
	}

};

/*
  rbtrees.h     (C) Jeremy Lee = Unorthodox Engineers 2006

  Implements a 'polymorphic' Red-Black tree system.

  Red-Black trees use a single bit of overhead per node in
  addition to the usual requirements of a binary tree of it's
  type. ('parent', 'left', 'right' links)

  Red-Black trees are self-balancing structures with minimal
  overhead, can search for exact or nearest matches, and performs
  all major operations in amortized log(n) time with bounded worst-
  case scenarios. They are elegant and subtle data structures,

  Their main downside is their complexity to code. While this generally
  gets handled by a library designer, they can easily cause system bloat
  as new code branches are inlined to satisfy different data types or
  comparison methods.

  These routines act a little strangely, using 'Indexes' to access pointer
  values from an array view of the node, rather than through a structure.
  Indexes are passed directly in the parameters of most routines. This is 
  necessary to allow a primitive but fast form of 'polymorphism', so the 
  same code path can manage trees of radically different structures. 

 */



class RedBlackTree {
private:
	// abstract methods that must be provided by real implementations
	virtual word parent(word node) = 0;
	virtual word left(word node) = 0;
	virtual word right(word node) = 0;
	virtual bool is_red(word node) = 0;

	virtual void set_parent(word node, word link) = 0;
	virtual void set_left(word node, word link) = 0;
	virtual void set_right(word node, word link) = 0;
	virtual void set_red(word node) = 0;
	virtual void set_black(word node) = 0;
	// virtual static void set_color(word node, bool red) = 0;

	virtual word tree_root() = 0;
	virtual void set_root(word node) = 0;

	virtual int node_compare(word lnode, word rnode) = 0;
	virtual int key_compare(word lnode, word rkey) = 0;

public:  
	typedef word NodeView;
	static const word null = 0;
	// match inclusion flags
	static const int MATCH_BEFORE =  -1;
	static const int MATCH_EXACT  =   0;
	static const int MATCH_AFTER  =   1;

	/*
      From an existing node, return the next tree node in sort order.
        time: 2log(n=tree size) worst case, log(log(n)) typical case. 
	 */
	NodeView tree_next_node(NodeView node) { 
		// it's either the leftmost descendant on our right, or the first ancestor
		// for which we're on the left.
		if( right(node) == null ) {
			while( (node != null) && ( parent(node) == null || node == right(parent(node)) )) { 
				node = parent(node); 
			}
			if(node == null) return null; else return parent(node);
		} else {
			return leftmost( right(node) );
		}
	}

	/*
      From an existing node, return the previous tree node in sort order
        time: 2log(n=tree size) worst case, log(log(n)) typical case. 
	 */
	NodeView tree_prev_node(NodeView node) {
		// it's either the rightmost descendant on our left, or the first ancestor
		// for which we're on the right
		if(left(node) == null ) {
			while( (node != null) && (parent(node) == null || node == left(parent(node)) )) { 
				node = parent(node); 
			}
			if(node == null) return null; else return parent(node);
		} else {
			return rightmost( left(node) );
		}
	}


	/*  seek comparable data using standard binary tree recursion method. In the case of
      the data not having an exact match, there is the option to have the seek return the node
      either before or after the point where the match should have been. All three match cases
      (Before, After, and Exact) have cases which return null; seeking the node before the missing
      first in the tree, or after the last, or the plain case of seeking an exact match which isn't there. */
	NodeView tree_seek_node(word key, int match) {
		// is there anything in the tree?
		NodeView n = tree_root();
		// seek recurse down the tree
		while( true ) {
			if( n==null ) return null;
			// compare the node's storage value with the one we want
			// using the compare callback
			int r = key_compare( n, key );
			// what was the result?
			if( r==0 ) {
				// an exact match!
				return n;
			} else if( r>0 ) {
				// seek data is less than the n. if(it has a left child)  recurse
				if( left(n)==null ) {
					// no child. return best guess (if(allowed) or null
					switch( match ) {
						case MATCH_BEFORE: return tree_prev_node(n);
						case MATCH_AFTER:  return n;
					}
					return null;
				} else n = left(n);
			} else {
				// seek data is greater than the n. if(it has a right child)  recurse
				if( right(n)==null ) {
					// no child. return best guess (if(allowed) or null
					switch( match ) {
						case MATCH_BEFORE: return n;
						case MATCH_AFTER:  return tree_next_node(n);
					}
					return null;
				} else n = right(n);
			}
		}
	}

	/* attempt to insert a new node into the tree. The node must already be
       filled with Comparable data, but all it's links should be null..
       return the conflict node on failure, or null on success.

      pattern:
        if( (conflict = tree_insert_node(newnode))!=0 ) {
          // our node was bounced because the key already existed
          delete newndode; // make sure something happens to it
        }
	 */
	NodeView tree_insert_node(NodeView node) {
		// clear the new nodes' branches
		set_parent(node,null);
		set_left(node,null);
		set_right(node,null);
		// is there anything in the tree?
		NodeView n = tree_root();
		if( n == null ) {
			// this becomes the root node.
			set_root(node);
			// which is colored black
			set_black(node);
			// all done
			return null;
		} else {
			// recurse down the tree to the insert n.
			while( 1 ) {
				// compare the node data to the insert value
				int r = node_compare( n, node );
				if( r==0 ) {
					// an exact match. 
					return n; // return the existing node
				} else if( r>0 ) {
					// less than. if it has a left child then recurse, otherwise insert
					NodeView cn = left(n);
					if( cn == null ) {
						// insert the new terminal node to the left
						set_left(n, node);
						set_parent(node, n);
						balance_postinsert(node);
						return null; // no conflicts
					} else  n = cn;
				} else {
					// greater than. if it has a right child then recurse, otherwise insert
					NodeView cn = right(n);
					if( cn == null ) {
						// insert the new terminal node to the right
						set_right(n, node); 
						set_parent(node, n);
						balance_postinsert(node);
						return null; // no conflicts
					} else  n = cn;
				}
			}
		}
	}

	/*
      Restore the Red-Black properties of the tree after a node has been inserted.
      the new node should not have any branches, unless you absolutely know what you are
      doing and can provide a tree with Red-Black properties that will not screw up the tree.
         time: 2log(n=tree size) worst case, log(log(n)) typical case. 
	 */
	void balance_postinsert(NodeView node) {
		//Serial.print(" post insert "); node_print(0,tree_root()); Serial.print("\n"); 
		NodeView y = null;
		NodeView p = parent(node);
		NodeView n;
		// does the node have a parent?
		if( p==null ) {
			// must be the root. it will get coloured Black at the end 
		} else {
			// the node starts life red, so it will not disturb the black-height property of the tree
			set_red( node );
			// did it have a grandparent?
			NodeView gp = parent(p);
			if( gp==null ) { 
				// not much of a tree. we're right under the root, meaning we and our possible sibling 
				// are both red, and the root will be coloured black at the end. Whew. that was easy!
			} else {
				// we have enough of a tree to balance. 
				// iterate while we are still under a red node (and not root)
				while( (node!=tree_root()) && is_red(p=parent(node)) ) {
					gp = parent(p);
					// Serial.print(" insert \n"); node_print(0,node); Serial.print("\n"); 
					// oops. our parent was red too. this will cause a height
					// violation, so we have to rebalance the tree.
					// which side was our parent on?
					if( p == left(gp) ) {
						// if nodes' parent is a left, y is nodes' right 'uncle'
						y = right(gp);
						if( (y!=null) && is_red(y) ) {
							// case 1 - change the colors
							set_black( y );
							set_black( p );
							set_red( gp );
							// iterate for the grandfather node
							node = gp; 
						} else {
							// y is a Black/sentinel node
							// what side of it's parent is the insert node?
							if( node==right(p) ) {
								// case 2 - move node up and rotate left
								node = p; // p = gp; gp = (p==null)?null:parent(p);
								rotate_left( node );
								p = parent(node); gp = parent(p);
								// as well as doing...
							}
							// case 3 - change parent and grandparent colours
							set_black( p );
							set_red( gp );
							// rotate the current grandparent node to the right, which should raise us in the tree
							rotate_right( gp );
						}
					} else {
						// if nodes' parent is a right, y is nodes' left 'uncle'
						y = left(gp);
						if( (y != null) && is_red(y) ) {
							// case 1 - change the colors
							set_black( y );
							set_black( p );
							set_red( gp );
							// iterate for the grandfather node
							node = gp;
						} else {
							// y is a Black/sentinel node
							if( node==left(p) ) {
								// and node is to the left
								// case 2 - move node up and rotate
								node = p;
								rotate_right( node );
								p = parent(node); gp = parent(p);
							}
							// case 3
							set_black( p );
							set_red( gp );
							// rotate the current grandparent node to the left
							rotate_left( gp );
						}
					}
				}
			}
		}
		// colour the new root
		set_black( tree_root() );
	}

	/* remove an existing node, and balance the remaining tree */
	void tree_remove_node(NodeView node) {
		// Serial.print(" before rm \n");
		// which child node will replace the deleted node?
		NodeView lchild = left(node);
		NodeView rchild = right(node);
		NodeView sub = null;
		// we need a situation where the node we are deleting does not have two
		// children, but only zero or one child that will replace it.
		if( lchild==null ) {
			// replace with right child, if it exists
			sub = rchild; 
		} else if( rchild==null ) {
			// replace with left child, since it must exist and the right does not.
			sub = lchild; 
		} else { 
			// both children exist. get the next node in sequence
			sub = leftmost(right( node ));
			// sub = tree_next_node( node );
			// exchange the nodes so that we temporarily violate tree order until the deletion, 
			exchange_nodes( node,sub ); 
			// but now the node to be deleted has only zero or one child to the right, 
			// and we shifted the problem further down the tree.
			sub = right(node);
		}
		// did we have a replacement child?
		if(sub == null) {
			// balance the tree before we remove this terminal node.
			if(!is_red(node)) { balance_postdelete( node ); }
			replace_node( node,null );
		} else {
			// balance the tree from the node that remains
			replace_node( node,sub );
			if(!is_red(sub)) { balance_postdelete( sub ); }
		}
		// unreference node pointers.
		set_parent(node, null); 
		set_left(node, null); 
		set_right(node, null);
	} 


	/*
      Restore the Red-Black and Counter properties 
      of the tree after a node has been deleted.
      X is either the replacement node, or the original node if it will be replaced will a null branch. 
        time: log(n=tree size) worst case, log(log(n)) typical case. 
	 */ 
	void balance_postdelete(NodeView node) {
		NodeView n = null;
		// iterate while we are black, until we are at the current root
		while( (node!=null) && !is_red( node ) ) {
			NodeView p = parent(node);
			NodeView sib_left = left(p); 
			NodeView sib_right = right(p); 
			// was the node on the left?
			bool on_left = (sib_left==node);
			// get the other sibling
			NodeView sibling = on_left ? sib_right : sib_left;
			if(sibling==null) {
				node = p;
			} else {
				if(on_left) {
					if(is_red(sibling)) {
						set_black(sibling);
						set_red(p);
						rotate_left(p);
						sibling = right(parent(node));
					}
					if(sibling==null) {
						node = parent(node);
					} else {
						NodeView sibling_l = left(sibling);  bool red_l = (sibling_l==null) ? false : is_red(sibling_l);
						NodeView sibling_r = right(sibling); bool red_r = (sibling_r==null) ? false : is_red(sibling_r);
						if( red_l || red_r ) {
							if(!red_r) {
								if(red_l) { set_black(sibling_l); }
								set_red(sibling);
								rotate_right(sibling);
								sibling = right(parent(node));
							}
							p = parent(node);
							if(sibling!=null) {
								sibling_r = right(sibling);
								if(is_red(p)) { set_red(sibling); } else { set_black(sibling); }
								if(sibling_r != null) { set_black(sibling_r); }
							}
							if(p==null) {
								Serial.print(" parent went missing \n");
							} else {
								set_black(p); 
								rotate_left(p);
							}
							node = null; // tree_root();
						} else {
							set_red(sibling);
							node = parent(node);
						}
					}
				} else {
					if(is_red(sibling)) {
						set_black(sibling);
						set_red(p);
						rotate_right(p);
						sibling = left(parent(node));
					}
					if(sibling==null) {
						node = parent(node);
					} else {
						NodeView sibling_l = left(sibling);  bool red_l = (sibling_l==null) ? false : is_red(sibling_l);
						NodeView sibling_r = right(sibling); bool red_r = (sibling_r==null) ? false : is_red(sibling_r);
						if( red_l || red_r ) {
							if(!red_l) {
								if(red_r) { set_black(sibling_r); }
								set_red(sibling);
								rotate_left(sibling);
								sibling = left(parent(node));
							}
							p = parent(node);
							if(sibling!=null) {
								sibling_l = left(sibling);
								if(is_red(p)) { set_red(sibling); } else { set_black(sibling); }
								if(sibling_l != null) { set_black(sibling_l); }
							}
							set_black(p);
							rotate_right(p);
							node = null; // tree_root();
						} else {
							set_red(sibling);
							node = parent(node);
						}
					}
				}
			}
		}
		// colour the root node to black
		node = tree_root();
		if(node!=null) { set_black( node ); }
	}


	/*
      Replace a node in the tree with another node. Used from within the
      delete_node function.

      This removes a node from the tree, and puts a replacement node n in its place
      by updating the parent pointers. Also handles the cases where the node
      was root, and the replacement node is null.

      Does not affect order. The algorithm is assumed to know what it's doing.
      No compares or colour checks are done at this stage.
	 */
	void replace_node(NodeView node, NodeView sub) {
		// was the old node root?
		if( node == tree_root()) {
			// is the new node null?
			if( sub!=null ) set_parent(sub, null);
			set_root( sub );
			//Serial.print("\n root replacement \n");
		} else {
			// is the new node null?
			NodeView p = parent(node);
			if( sub!=null ) set_parent(sub, p);
			// which side of the parent was the node on?
			if( left(p)==node ) { set_left(p, sub); } else { set_right(p, sub); }
		}
	}


	/*  Exchanging two nodes in the tree usually violates some structural properties,
      (order, if nothing else) so unless it's being done by the core algorithms or
      to fix such a property, you shouldn't be using this method.  */
	void exchange_nodes(NodeView x, NodeView y) {
		NodeView n = null;
		// change the child links on the parents
		NodeView px = parent(x);
		NodeView py = parent(y);
		if( px == null) { set_root(y); } else if( left(px) == x ) { set_left(px, y); } else { set_right(px, y); }
		if( py == null) { set_root(x); } else if( left(py) == y ) { set_left(py, x); } else { set_right(py, x); }
		// swap the links to the parents
		set_parent(x, py); set_parent(y, px);
		// swap the links to the children
		n = left(x); set_left(x, left(y)); set_left(y, n);
		n = right(x); set_right(x, right(y)); set_right(y, n);
		// set the parent links on the children
		if( (n=left(x)) != null)  set_parent(n, x);
		if( (n=right(x)) != null) set_parent(n, x);
		if( (n=left(y)) != null)  set_parent(n, y);
		if( (n=right(y)) != null) set_parent(n, y);
		// swap the colors
		if(is_red(x)) {
			if(!is_red(y)) { set_black(x); set_red(y); }
		} else {
			if(is_red(y)) { set_red(x); set_black(y); }
		}    
	}

	/*
      Go along one 'side' of the tree until there are no more
      in that direction. if the side is left, right or parent,
      then the 'direction' of the search is leftmost, rightmost,
      or root. Good for finding the first and last tree entries.
      time: log(n=tree size)
	 */    
	NodeView leftmost(NodeView node) {
		NodeView next;
		while( (next = left(node)) != null ) { node = next; }
		return node;
	}

	NodeView rightmost(NodeView node) {
		NodeView next;
		while( (next = right(node)) != null ) { node = next; }
		return node;
	}

	/*
      Rotate the tree left about a node
      Does not perform colour updates
        time: constant
	 */
	void rotate_left(NodeView x) {
		// what is the parent, and to the right of X? 
		NodeView p = parent(x);
		NodeView y = right(x);
		// replace Y in X with Y's left sub-tree, if it exists
		NodeView n = left(y);
		set_right(x, n);
		if(n != null) {
			// update the parent
			set_parent(n, x);
		}
		// replace X with Y in the parent.
		set_parent(y, p);
		if( p==null ) { // <- think this is wrong
			// X was the root
			set_root(y); // <- nulls being used to replace the root from here
		} else {
			// X had a parent
			if( left(p)==x ) { set_left(p, y); } else { set_right(p, y); }
		}
		// Finally, put X on Y's left
		set_left(y, x); 
		set_parent(x, y);
	}

	/*
      Rotate the tree right about a node
      Does not perform colour updates
        time: constant
	 */
	void rotate_right(NodeView x) {
		// what is the parent, and to the left of X? 
		NodeView p = parent(x);
		NodeView y = left(x);
		// Turn y's right sub-tree into x's left sub-tree
		NodeView n = right(y);
		set_left(x, n);
		if( n != null ) {
			// update the parent
			set_parent(n, x);
		}
		// replace X with Y in the parent.
		set_parent(y, p);
		if( p==null ) {
			// X was the root
			set_root(y);
		} else {
			// X had a parent
			if( left(p)==x ) { set_left(p, y); } else { set_right(p, y); }
		}
		// Finally, put x on y's right
		set_right(y, x);
		set_parent(x, y);
	}


	// debug utility functions

	void indent_print(int indent) {
		while(indent-- > 0) Serial.print("  ");
	}

	virtual void node_print(int indent, word node) =0;


};

/*

 */
struct MapNode {
	word parent;
	word left;
	word right;
	word meta; // color bit plus key

	MapNode() {
		this->meta = 0;
	}

	MapNode(word key) {
		this->meta = key & 0x7fff;
	}

	word get_key() {
		return this->meta & 0x7fff;
	}

	void set_key(word key) {
		this->meta = (key & 0x7fff) | (this->meta & 0x8000); // keep the color bit
	}
};

/*

 */
class Map: public RedBlackTree {
private:
	word root;

	// abstract methods that must be provided by real implementations
	virtual word parent(word node) { return ((MapNode *)node)->parent; }
	virtual word left(word node) { return ((MapNode *)node)->left; }
	virtual word right(word node) { return ((MapNode *)node)->right; }
	virtual bool is_red(word node) { return (((MapNode *)node)->meta & 0x8000)!=0; }

	virtual void set_parent(word node, word link) { ((MapNode *)node)->parent = link; }
	virtual void set_left(word node, word link)  { ((MapNode *)node)->left = link; }
	virtual void set_right(word node, word link)  { ((MapNode *)node)->right = link; }
	virtual void set_red(word node)  { ((MapNode *)node)->meta |= 0x8000; }
	virtual void set_black(word node)  { ((MapNode *)node)->meta &= 0x7FFF; }

	virtual word tree_root() { return root; }
	virtual void set_root(word node) { root = node; }

	virtual int node_compare(word lnode, word rnode) {   
		word lkey = ((MapNode *)lnode)->meta & 0x7FFF;
		word rkey = ((MapNode *)rnode)->meta & 0x7FFF;
		return lkey - rkey;
	};

	virtual int key_compare(word lnode, word rkey) {
		word lkey = ((MapNode *)lnode)->meta & 0x7FFF;
		return lkey - rkey;
	};

	virtual void node_print(int indent, word node) {
		indent_print(indent);
		if(node==null) {
			Serial.print("[empty]\n");
		} else {
			Serial.print("[node "); Serial.print(node);
			Serial.print(" parent:"); Serial.print(((MapNode *)node)->parent);
			Serial.print((((MapNode *)node)->meta & 0x8000) ? " red" : " black"); 
			Serial.print("]:"); Serial.print(((MapNode *)node)->meta & 0x7FFF);
			Serial.print("\n");
			node_print(indent+1, ((MapNode *)node)->left );
			node_print(indent+1, ((MapNode *)node)->right );
		}
	}

public:
	Map() {
		// clear root node
		root = 0;
	}
	MapNode * seek(word key, int match) {
		return (MapNode *)tree_seek_node(key, match); 
	}
	MapNode * seek(word key) {
		return (MapNode *)tree_seek_node(key, MATCH_EXACT); 
	}
	word add(MapNode * node) {
		return tree_insert_node((word)node);
	}
	word rm(MapNode * node) {
		tree_remove_node((word)node);
	}
	bool rm(word key) {
		word node = tree_seek_node(key, MATCH_EXACT);
		if(node) {
			tree_remove_node(node);
			delete (MapNode *)node;
			return true;
		}
		return false;
	}

	MapNode * first() { return (root==null) ? null : (MapNode *)leftmost(root); }
	MapNode * last() { return (root==null) ? null : (MapNode *)rightmost(root); }
	MapNode * next(MapNode * node) { return (MapNode *)tree_next_node((NodeView)node); }
	MapNode * prev(MapNode * node) { return (MapNode *)tree_prev_node((NodeView)node); }

	void print() {
		node_print(0,root);
	}
};


#endif