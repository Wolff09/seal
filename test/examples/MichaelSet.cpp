#name "Michael's Set"
#cavespec "set_spec.cav"
// #cavelin 'prover_opts imp_var = "CC_.Tail.next";'

struct Node {
	data_t val;
	bool marked;
	Node* next;
}

Node* Head;

extern void retire(Node* ptr);
extern void protect1(Node* ptr);
extern void protect2(Node* ptr);
extern void protect3(Node* ptr);

void init() {
	Head = malloc;
	Head->next = NULL;
}

void enqueue(data_t value) {
	Node* tail, next, node;

	node = malloc;
	node->val = value;
	node->next = NULL;

	while (true) {
		tail = Tail;
		protect1(tail);

		if (tail == Tail) {
			next = tail->next;

			if (tail == Tail) {
				if (next != NULL) {
					CAS(Tail, tail, next);

				} else {
					if (CAS(tail->next, NULL, node)) {
						CAS(Tail, tail, node);
						break;
					}
				}
			}
		}
	}
}

data_t dequeue() {
	Node* head, next, tail;
	data_t result;
	bool flag;

	while (true) {
		head = Head;
		protect1(head);

		if (head == Head) {
			tail = Tail;
			next = head->next;
			protect2(next);

			if (head == Head) {
				if (next == NULL) {
					result = EMPTY;
					break;

				} else {
					if (head == tail) {
						CAS(Tail, tail, next);

					} else {
						result = next->val;
						atomic {
							if (CAS(Head, head, next)) {
								retire(head); // moved left to avoid CAVE imprecision
								flag = true;
							} else {
								flag = false;
							}
						}
						if (flag) {
							// retire(head);
							break;
						}
					}
				}
			}
		}
	}
	return result;
}



struct Node {
	data_t val;
	bool marked;
	ptr_t next;
};

Node* Head;

bool find(key_t key) {
	Node prev, cur, next;
	bool prev_mark, cur_mark;
	data_t cur_key;

	while (true) {
		prev = Head;
		atomic {
			cur = prev->next;
			prev_mark = prev->marked;
		}

		while (true) {

			if (cur == NULL) {
				return false;
			}

			atomic {
				next = cur->next;
				cur_mark = cur->marked;
			}
			cur_key = cur->key;

			// next field of a node contains its mark
			// bool prev_mark = cur.mark;
			// bool cur_mark = next.mark;

			if (prev_mark || cur != prev->next) {
				// continue with outer loop
				break;
			}

			if (!cur_mark) {
				if (cur_key >= key) {
					return cur_key == key;
				} else {
					// re-reading at prev may give
					//  - uninteded value from intermediate reuse
					//  - could render prev strongly invalid in case of a free
					atomic {
						prev = cur;
						prev_mark = cur_mark;
					}
				}

			} else {
				atomic {
					tmpPtr = prev->next;
					tmpBool = prev->marked;
					choose {
						assume(tmpPtr == cur);
						assume(tmpBool == cur_mark);
						prev->next = next;
						prev->marked = false;
					}{
						choose {
							assume(tmpPtr != cur);
							break;
						}{
							assume(tmpBool != cur_mark);
							break;
						}
					}
				}
			}

			atomic {
				cur = next;
			}
		}
	}
}


bool Search(key_t key) {
	return find(key);
}



