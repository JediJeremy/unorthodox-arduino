/*
  HAL Queue Entries
 */
#ifndef UNORTHODOX_QUEUES_H
#define UNORTHODOX_QUEUES_H

class HALNode {
public:
	HALNode * next;
};

/*
  Interrupt-safe Queues
 */
class HALQueue : private HALNode {
private:
	// class static
	static HALQueue * _interrupt_queue[16];
	static HALNode *  _interrupt_node[16];
	static byte _interrupt_level;
public:
	// constructor
	HALQueue() { 
		next = 0;
		head = this;
		tail = this;
	}
	// queue methods
	void enqueue(HALNode * node) {
		hal_queue_enqueue(this, node);
	}
	HALNode * dequeue() {
		return hal_queue_dequeue(this,this);
	}
	static void set_interrupt(byte n) { _interrupt_level = n; }
private:
	// 
	HALNode * head;
	HALNode * tail;

#define null 0

	static HALNode * hal_chain_follow(HALNode * node) {
		// follow the previous level's chain to the last entry
		HALNode * next = node->next;
		while(next != null) {
			node = next;
			next = node->next;
		}
		return node;
	}

	// search for the most recent interrupt level which was also modifying this queue
	static int hal_queue_seeklevel(HALQueue * queue, int level) {  
		while( --level >= 0 ) {
			if( _interrupt_queue[level] == queue ) return level;
		}
		return -1;
	}

	// which node (reachable from the global table or chain tail) references a given node?
	static HALNode * hal_chain_anchor(HALQueue * queue, HALNode * node, int level ) {
		while( true ) {
			// find the next level down
			level = hal_queue_seeklevel(queue, level);
			// which chain represents this level?
			HALNode * link = ( level < 0 ) ? queue->tail : _interrupt_node[level];
			// go through the chain and look for the node
			while( link != null ) {
				HALNode * next = link->next;
				if(next == node) return link;
				link = next;
			}
			// return if we ran out of levels to check, otherwise loop
			if( level < 0 ) return null;
		}
	}

	static void hal_queue_enqueue(HALQueue * queue, HALNode * node ) {
		// what interrupt level are we at?
		int level = _interrupt_level;
		// ensure the chain references for this level start empty
		// so there is never an invalid pair.
		// assert( _interrupt_queue[level] == null );
		// assert( _interrupt_node[level] == null );
		// fill them with our values. order is very important in case we are interrupted
		_interrupt_node[level] = node;
		_interrupt_queue[level] = queue;
		// interrupts will now append to our level. We can examine and correct the table in peace.
		int prev_level = hal_queue_seeklevel(queue, level);
		HALNode * prev;
		while( prev_level != -1 ) {
			// is the previous level node the tail?
			prev = _interrupt_node[prev_level];
			if( queue->tail == prev ) {
				// clear the pipe entry from the table (but not the node)
				_interrupt_queue[prev_level] = null;
				// go around again with the next level down
				prev_level = hal_queue_seeklevel(queue, level);
			} else break;
		}
		// are we left with the simple case of no previous active levels?
		if( prev_level == -1 ) {
			// catch the pipe's tail, and link ourselves to the end
			HALNode * last = hal_chain_follow(queue->tail);
			last->next = node;
			// update the pipe tail, now that we can catch ours
			last = hal_chain_follow(node);
			_interrupt_node[level] = last;
			queue->tail = last;
			// higher level interrupts may now clear our level
		} else {
			// has the previous level linked itself into a chain yet?
			HALNode * anchor = hal_chain_anchor(queue,prev, prev_level);
			HALNode * link;
			HALNode * last;
			if( anchor == null ) {
				// stalled. replace the previous levels' chain
				link = prev->next;
				prev->next = node;
				// catch our tail and link to the old chain
				last = hal_chain_follow(node);
				last->next = link;
				// the lower continuance will catch our appended nodes
			} else {
				// anchored. replace the existing chain following the anchor node
				link = anchor->next;
				anchor->next = node;
				// we are now reachable. catch our own tail, and link the chain
				last = hal_chain_follow(node);
				last->next = link;
				// can we pre-update the pipe tail? (optional, but useful)
				if( anchor == queue->tail ) {
					_interrupt_node[level] = last;
					queue->tail = last;
				}
			}
		}
		// clear the global level entries, during which the tail may move past us
		_interrupt_queue[level] = null;
		_interrupt_node[level] = null;
	}

	static HALNode * hal_queue_dequeue(HALQueue * queue, HALNode * sentinel) {
		while(true) {
			// get the head and tail of the queue and the head's next
			HALNode * head = queue->head;
			HALNode * tail = queue->tail;
			HALNode * next = head->next;
			// return null if the pipe has only one proper node
			if(head == tail) return null;
			if(next == null) return null;
			// we have a new queue head
			queue->head = next;    
			// remove the old link, for safety's sake
			head->next = null;
			// was it the sentinel?
			if(head == sentinel) {
				// recycle the sentinel
				hal_queue_enqueue(queue, sentinel);
				// loop around again
			} else {
				// we have dequeued the entry sucessfully
				return head;
			}
		}
	}

#undef null
};

byte HALQueue::_interrupt_level = 0;
HALQueue * HALQueue::_interrupt_queue[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
HALNode * HALQueue::_interrupt_node[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


#endif