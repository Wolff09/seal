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
	Head->marked = false;
}


bool contains(data_t key) {
	Node* prev, cur, next, tmpPtr;
	bool prevMark, curMark, result, retry, tmpBool;
	data_t curKey;

	// BEGIN FIND
	retry = true;
	while (retry) {

		atomic {
			prev = Head;
			cur = prev->next;
			prevMark = prev->marked;
		}
		protect2(cur);
		choose {
			atomic {
				assume(prevMark == false);
				tmpPtr = prev->next;
				assume(cur == tmpPtr);
			}

			while (true) {

				if (cur == NULL) {
					// return false
					result = false;
					retry = false;
					break;
				} else {

					atomic {
						next = cur->next;
						curMark = cur->marked;
					}
					curKey = cur->val;

					protect1(next);
					choose {
						atomic {
							tmpPtr = cur->next;
							tmpBool = cur->marked;
							assume(curMark == tmpBool);
							assume(next == tmpPtr);
						}

						choose {
							atomic {
								assume(prevMark == false);
								tmpPtr = prev->next;
								assume(cur == tmpPtr);
							}

							if (!curMark) {
								if (curKey >= key) {
									// return curKey == key
									if (curKey == key) {
										result = true;
									} else {
										result = false;
									}
									retry = false;
									break;
								} else {
									atomic {
										prev = cur;
										prevMark = curMark;
									}
									protect3(cur);
								}

							} else {
								atomic {
									tmpPtr = prev->next;
									tmpBool = prev->marked;
									choose {
										assume(tmpPtr == cur);
										assume(tmpBool == false);
										prev->next = next;
										tmpBool = false;
										prev->marked = tmpBool;
										retire(cur);
									}{
										// continue with outer loop
										break; // TODO: correct?
										// choose {
										// 	assume(tmpPtr != cur);
										// 	break;
										// }{
										// 	assume(tmpBool != curMark);
										// 	break;
										// }
									}
								}
							}

							atomic {
								cur = next;
							}
							protect2(next);

						}{
							// continue with outer loop
							break;
						}

					}{
						// continue with outer loop
						break;
					}
				}
			}
		}{
			assume(true);
		}
	}
	// END FIND

	return result;
}


bool add(data_t key) {
	Node* node;
	bool insertionResult;
	Node* prev, cur, next, tmpPtr;
	bool prevMark, curMark, result, retry, tmpBool;
	data_t curKey;

	node = malloc;
	node->val = key;
	tmpBool = false;
	node->marked = tmpBool;
	while (true) {
		// BEGIN FIND
		retry = true;
		while (retry) {

			atomic {
				prev = Head;
				cur = prev->next;
				prevMark = prev->marked;
			}
			protect2(cur);
			choose {
				atomic {
					assume(prevMark == false);
					tmpPtr = prev->next;
					assume(cur == tmpPtr);
				}

				while (true) {

					if (cur == NULL) {
						// return false
						result = false;
						retry = false;
						break;
					} else {

						atomic {
							next = cur->next;
							curMark = cur->marked;
						}
						curKey = cur->val;

						protect1(next);
						choose {
							atomic {
								tmpPtr = cur->next;
								tmpBool = cur->marked;
								assume(curMark == tmpBool);
								assume(next == tmpPtr);
							}

							choose {
								atomic {
									assume(prevMark == false);
									tmpPtr = prev->next;
									assume(cur == tmpPtr);
								}

								if (!curMark) {
									if (curKey >= key) {
										// return curKey == key
										if (curKey == key) {
											result = true;
										} else {
											result = false;
										}
										retry = false;
										break;
									} else {
										atomic {
											prev = cur;
											prevMark = curMark;
										}
										protect3(cur);
									}

								} else {
									atomic {
										tmpPtr = prev->next;
										tmpBool = prev->marked;
										choose {
											assume(tmpPtr == cur);
											assume(tmpBool == false);
											prev->next = next;
											tmpBool = false;
											prev->marked = tmpBool;
											retire(cur);
										}{
											// continue with outer loop
											break; // TODO: correct?
											// choose {
											// 	assume(tmpPtr != cur);
											// 	break;
											// }{
											// 	assume(tmpBool != curMark);
											// 	break;
											// }
										}
									}
								}

								atomic {
									cur = next;
								}
								protect2(next);

							}{
								// continue with outer loop
								break;
							}

						}{
							// continue with outer loop
							break;
						}
					}
				}
			}{
				assume(true);
			}
		}
		// END FIND
		if (result) {
			insertionResult = false;
			break;
		} else {
			node->next = cur;
			atomic {
				tmpPtr = prev->next;
				tmpBool = prev->marked;
				choose {
					assume(tmpPtr == cur);
					assume(tmpBool == false);
					prev->next = node;
					tmpBool = false;
					prev->marked = tmpBool;
					insertionResult = true;
					break;
				}{
					assume(true);
				}
			}
		}
	}

	return insertionResult;
}
