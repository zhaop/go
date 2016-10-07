#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <wchar.h>

#include "players.h"
#include "players/teresa.h"
#include "utils.h"

// To use these macros, a variable "tree" must be defined
// Matching undefs at bottom of this file
#define NODE_NULL 0										// Set nodes to NODE_NULL instead of 0 for clarity
#define NODE_PARENT(node) (tree->parent[(node)])
#define NODE_SIBLING(node) (tree->sibling[(node)])
#define NODE_CHILD(node) (tree->child[(node)])
#define NODE_PL(node) (tree->pl[(node)])
#define NODE_MV(node) (tree->mv[(node)])
#define NODE_WINS(node) (tree->wins[(node)])
#define NODE_VISITS(node) (tree->visits[(node)])
#define NODE_PWIN(node) (tree->pwin[(node)])
#define NODE_SQLG_VISITS(node) (tree->sqlg_visits[(node)])
#define NODE_RSQRT_VISITS(node) (tree->rsqrt_visits[(node)])
#define NODE_UNEXPLORED_MOVES(node) (tree->unexplored_moves[(node)])
#define NODE_UNEXPLORED_COUNT(node) (tree->unexplored_count[(node)])

static unsigned int teresa_node_count = 0;

static inline void teresa_node_init(teresa_tree* tree, teresa_node node) {
	NODE_PARENT(node) = NODE_NULL;
	NODE_SIBLING(node) = NODE_NULL;
	NODE_CHILD(node) = NODE_NULL;
	NODE_PL(node) = EMPTY;
	NODE_MV(node) = MOVE_NULL;
	NODE_WINS(node) = 0;
	NODE_VISITS(node) = 0;
	NODE_PWIN(node) = NAN;
	NODE_SQLG_VISITS(node) = NAN;
	NODE_RSQRT_VISITS(node) = NAN;
	NODE_UNEXPLORED_MOVES(node) = NULL;
	NODE_UNEXPLORED_COUNT(node) = 0;
}

static inline float node_pwin(teresa_tree* tree, teresa_node nd) {
	return isnan(NODE_PWIN(nd)) ? (NODE_PWIN(nd) = (float)NODE_WINS(nd)/NODE_VISITS(nd)) : NODE_PWIN(nd);
}

static inline float node_sqlg_visits(teresa_tree* tree, teresa_node nd) {
	return isnan(NODE_SQLG_VISITS(nd)) ? (NODE_SQLG_VISITS(nd) = sqrt(log((float)NODE_VISITS(nd)))) : NODE_SQLG_VISITS(nd);
}

static inline float node_rsqrt_visits(teresa_tree* tree, teresa_node nd) {
	return isnan(NODE_RSQRT_VISITS(nd)) ? (NODE_RSQRT_VISITS(nd) = 1 / sqrt((float)NODE_VISITS(nd))) : NODE_RSQRT_VISITS(nd);
}

static inline teresa_node tree_alloc(teresa_tree* tree) {
	teresa_node node = tree->freeroot;
	tree->freeroot = NODE_SIBLING(node);
	return node;
}

static inline teresa_node teresa_node_create(teresa_tree* tree) {
	teresa_node node = tree_alloc(tree);
	++teresa_node_count;
	return node;
}

static inline void tree_free(teresa_tree* tree, teresa_node nd) {
	NODE_SIBLING(nd) = tree->freeroot;
	tree->freeroot = nd;
}

// Destroys a node's siblings and children
static void teresa_node_destroy_recursive(teresa_tree* tree, teresa_node nd) {
	if (NODE_CHILD(nd)) {
		teresa_node_destroy_recursive(tree, NODE_CHILD(nd));
	}
	if (NODE_SIBLING(nd)) {
		teresa_node_destroy_recursive(tree, NODE_SIBLING(nd));
	}
	if (NODE_UNEXPLORED_COUNT(nd)) {
		free(NODE_UNEXPLORED_MOVES(nd));
	}
	tree_free(tree, nd);
	--teresa_node_count;
}

// Destroy a node and its children
static inline void teresa_node_destroy(teresa_tree* tree, teresa_node nd) {
	if (NODE_CHILD(nd)) {
		teresa_node_destroy_recursive(tree, NODE_CHILD(nd));
	}
	if (NODE_UNEXPLORED_COUNT(nd)) {
		free(NODE_UNEXPLORED_MOVES(nd));
	}
	tree_free(tree, nd);
	--teresa_node_count;
}

static inline teresa_node teresa_node_sibling(teresa_tree* tree, teresa_node nd, int idx) {
	for (int i = 0; i < idx; ++i) {
		nd = NODE_SIBLING(nd);
		if (!nd) {
			return NODE_NULL;
		}
	}
	return nd;
}

// Initialize tree: empty decision tree, flat free tree
static void teresa_init_tree(teresa_tree* tree) {
	tree->root = NODE_NULL;
	
	teresa_node node;

	// Leave index 0 unitialized, so valgrind could catch anything accessing it
	// Actually, each node should initialize itself
	// memset(tree->parent + 1, NODE_NULL, sizeof(tree->parent[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->sibling + 1, NODE_NULL, sizeof(tree->sibling[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->child + 1, NODE_NULL, sizeof(tree->child[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->pwin + 1, NODE_NULL, sizeof(tree->pwin[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->sqlg_visits + 1, NODE_NULL, sizeof(tree->sqlg_visits[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->rsqrt_visits + 1, NODE_NULL, sizeof(tree->rsqrt_visits[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->wins + 1, NODE_NULL, sizeof(tree->wins[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->visits + 1, NODE_NULL, sizeof(tree->visits[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->mv + 1, NODE_NULL, sizeof(tree->mv[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->pl + 1, NODE_NULL, sizeof(tree->pl[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->unexplored_moves + 1, NODE_NULL, sizeof(tree->unexplored_moves[0]) * (TERESA_MAX_NODES - 1));
	// memset(tree->unexplored_count + 1, NODE_NULL, sizeof(tree->unexplored_count[0]) * (TERESA_MAX_NODES - 1));

	// Initialize free tree
	tree->freeroot = 1;
	for (node = 1; node < TERESA_MAX_NODES-2; ++node) {
		NODE_SIBLING(node) = node+1;
	}
	NODE_SIBLING(TERESA_MAX_NODES-1) = NODE_NULL;

}

static void teresa_init_params(void* params) {
	teresa_tree* tree = ((teresa_params*) params)->tree;
	if (!tree) {
		tree = malloc(sizeof(teresa_tree));
		((teresa_params*) params)->tree = tree;
		teresa_init_tree(tree);

		teresa_node root = teresa_node_create(tree);
		assert(root);
		teresa_node_init(tree, root);
		tree->root = root;
	}
}

// Generate list of unexplored moves for a node, return number of moves
static int teresa_generate_unexplored_moves(state* st, teresa_tree* tree, teresa_node nd) {
	move list[NMOVES];
	int n = chess_get_reasonable_moves(st, list);

	move* unexplored_moves = malloc(n * sizeof(move));
	assert(unexplored_moves);

	NODE_UNEXPLORED_COUNT(nd) = n;
	NODE_UNEXPLORED_MOVES(nd) = unexplored_moves;	// BUG Check for leaked memory?

	for (int i = 0; i < n; ++i) {
		unexplored_moves[i] = list[i];
	}

	return n;
}

// Assumes NODE_UNEXPLORED_COUNT(nd) >= 1
static move teresa_extract_unexplored_move(teresa_tree* tree, teresa_node nd) {
	const uint16_t count = NODE_UNEXPLORED_COUNT(nd);

	int r = RANDI(0, count);
	move mv = NODE_UNEXPLORED_MOVES(nd)[r];

	// Swap with last element & shorten array
	NODE_UNEXPLORED_MOVES(nd)[r] = NODE_UNEXPLORED_MOVES(nd)[count-1];
	NODE_UNEXPLORED_COUNT(nd) = count - 1;

	if (NODE_UNEXPLORED_COUNT(nd) <= 0) {
		free(NODE_UNEXPLORED_MOVES(nd));
		NODE_UNEXPLORED_MOVES(nd) = NODE_NULL;
	}

	return mv;
}

// Creates a new child node for given move & inserts it correctly into tree
static teresa_node teresa_node_create_child(teresa_tree* tree, teresa_node parent, move* mv) {
	teresa_node child = teresa_node_create(tree);
	assert(child);

	teresa_node_init(tree, child);

	NODE_PARENT(child) = parent;
	if (NODE_CHILD(parent)) {	// Insert into existing child list
		NODE_SIBLING(child) = NODE_CHILD(parent);
		NODE_CHILD(parent) = child;
	} else {	// Currently only child
		NODE_CHILD(parent) = child;
	}
	NODE_PL(child) = color_opponent(NODE_PL(parent));
	NODE_MV(child) = *mv;

	return child;
}

static teresa_node teresa_select_best_child(teresa_tree* tree, teresa_node current, teresa_params* params, bool friendly_turn, float lower_bound) {
	const float C = params->C;
	const float FPU = params->FPU;

	const float k = (NODE_VISITS(current) == 0) ? 1.0 : C * node_sqlg_visits(tree, current);

	float UCBs[NMOVES];

	int i = 0;
	float max_UCB = lower_bound;
	int nmax_UCB = 0;
	teresa_node selected_child = NODE_NULL;
	teresa_node child = NODE_CHILD(current);
	while (child) {
		if (NODE_VISITS(child)) {
			if (friendly_turn) {
				UCBs[i] = node_pwin(tree, child) + k * node_rsqrt_visits(tree, child);
			} else {
				UCBs[i] = 1 - node_pwin(tree, child) + k * node_rsqrt_visits(tree, child);
			}
		} else {
			UCBs[i] = FPU;
		}

		// Count max values
		if (UCBs[i] == max_UCB) {
			++nmax_UCB;
			selected_child = child;
		} else if (UCBs[i] > max_UCB) {
			max_UCB = UCBs[i];
			nmax_UCB = 1;
			selected_child = child;
		}

		++i;
		child = NODE_SIBLING(child);
	}

	if (max_UCB == lower_bound && nmax_UCB == 0) {
		// All children are below lower_bound
		return NODE_NULL;
	} else if (nmax_UCB == 1) {
		// Found child
		return selected_child;
	} else {
		// Found many children with same UCB
		int idx_max = pick_value_f(UCBs, i, max_UCB, nmax_UCB);
		assert(idx_max != -1);
		return teresa_node_sibling(tree, NODE_CHILD(current), idx_max);
	}
}

// Return most visited child (the one we're most certain of?)
static teresa_node teresa_select_most_visited_child(teresa_tree* tree, teresa_node current) {
	float visits[NMOVES];	// float because pick_value_f only takes floats (overkill?)

	int i = 0;
	float max_visits = 0;
	int nmax = 0;
	teresa_node selected_child = NODE_NULL;
	teresa_node child = NODE_CHILD(current);
	while (child) {
		float visit = (float) NODE_VISITS(child);
		visits[i] = visit;

		// Count max values
		if (visit == max_visits) {
			++nmax;
		} else if (visit > max_visits) {
			max_visits = visit;
			nmax = 1;
			selected_child = child;
		}

		++i;
		child = NODE_SIBLING(child);
	}

	if (nmax == 1) {
		return selected_child;
	} else {
		int idx_max = pick_value_f(visits, i, max_visits, nmax);
		assert(idx_max != -1);
		return teresa_node_sibling(tree, NODE_CHILD(current), idx_max);
	}
}

static void teresa_print_heatmap(state* st, teresa_tree* tree, teresa_node nd) {
	double values[NMOVES];
	move mvs[NMOVES];

	int i = 0;
	teresa_node child = NODE_CHILD(nd);
	while (child) {
		values[i] = node_pwin(tree, child);
		mvs[i] = NODE_MV(child);

		++i;
		child = NODE_SIBLING(child);
	}

	chess_print_heatmap(st, mvs, values, i);
}

static void teresa_destroy_all_children_except_one(teresa_tree* tree, teresa_node parent, teresa_node keep) {
	teresa_node next;
	teresa_node current = NODE_CHILD(parent);
	do {
		next = NODE_SIBLING(current);
		if (current == keep) {
			NODE_SIBLING(current) = NODE_NULL;
			NODE_PARENT(current) = NODE_NULL;
		} else {
			teresa_node_destroy(tree, current);
		}
		current = next;
	} while (current);
}

#define PARAM_C 0.5

void pshort(teresa_tree* tree, teresa_node nd) {
	// static teresa_params test_params = {0, 0.5, 1.1, 0, 0};

	wprintf(L"{");
	
	if (nd) {
		move_print(&NODE_MV(nd));

		float k = 1;
		if (NODE_PARENT(nd) && NODE_VISITS(NODE_PARENT(nd)) != 0) {
			k = PARAM_C * node_sqlg_visits(tree, NODE_PARENT(nd));
		}
		wprintf(L", %d/%d; %.3f, %.3f", NODE_WINS(nd), NODE_VISITS(nd),
			((double)NODE_WINS(nd)/NODE_VISITS(nd)) + k / sqrt(NODE_VISITS(nd)), 1 - ((double)NODE_WINS(nd)/NODE_VISITS(nd)) + k / sqrt(NODE_VISITS(nd)));
	}
	wprintf(L"}");
}

void p(teresa_tree* tree, teresa_node nd) {
	wprintf(L"teresa_node (%x) {\n", nd);
	wprintf(L"  parent:\n    ");
	pshort(tree, NODE_PARENT(nd));
	
	wprintf(L"\n  self:\n    ");
	pshort(tree, nd);
	
	wprintf(L"\n  siblings:");
	teresa_node current = nd;
	while (NODE_SIBLING(current)) {
		wprintf(L"\n    ");
		pshort(tree, NODE_SIBLING(current));
		current = NODE_SIBLING(current);
	}
	
	wprintf(L"\n  child:\n    ");
	pshort(tree, NODE_CHILD(nd));

	wprintf(L"\n}\n");
}

static int int_desc_cmp(const void* a, const void* b) {
	return *((int*)b) - *((int*)a);
}

static void graph_tree(FILE* f, teresa_tree* tree, teresa_node nd, int depth, int cutoff) {
	if (!nd) return;
	
	wchar_t mv_str[3];
	move_sprint(mv_str, &NODE_MV(nd));

	color pl = NODE_PL(nd);
	char color_c = (pl == BLACK) ? 'b' : (pl == WHITE ? 'w' : 'n');

	fprintf(f, "{\"id\":\"%x\",\"player\":\"%c\",\"move\":\"%ls\",\"visits\":%d,\"wins\":%d", nd, color_c, mv_str, NODE_VISITS(nd), NODE_WINS(nd));

	if (depth > 0 && NODE_CHILD(nd)) {
		--depth;
		teresa_node child;

		// Find highest visits first
		int node_visits[NMOVES];
		int i = 0;
		child = NODE_CHILD(nd);
		while (child) {
			node_visits[i] = NODE_VISITS(child);
			++i;
			child = NODE_SIBLING(child);
		}
		qsort(node_visits, i, sizeof(int), int_desc_cmp);
		unsigned int thresh = max(node_visits[min(cutoff, i)-1], 40);
		
		if (i) {
			child = NODE_CHILD(nd);
			bool nothing_printed = true;
			while (child) {
				if (NODE_VISITS(child) >= thresh) {
					if (nothing_printed) {
						fprintf(f, ",\"children\":[\n");
						nothing_printed = false;
					} else {
						fprintf(f, ",\n");
					}
					graph_tree(f, tree, child, depth, cutoff);
				}
				child = NODE_SIBLING(child);
			}
			if (!nothing_printed) {
				fprintf(f, "]");
			}
		}
	}

	fprintf(f, "}");
}

void g(teresa_tree* tree, teresa_node root) {
	g2(tree, root, "graph1.json", 7, 7);
}

void g2(teresa_tree* tree, teresa_node root, const char* path, int depth, int thresh) {

	if (!root) {
		wprintf(L"root is NULL\n");
		return;
	}

	const char fmode = 'w';
	
	FILE* f = fopen(path, &fmode);
	graph_tree(f, tree, root, depth, thresh);
	
	fclose(f);
}

// params.N, params.C must be defined
move_result teresa_play(player* self, state* st0, move* mv) {
	color me = st0->nextPlayer;
	color notme = (me == BLACK) ? WHITE : BLACK;

	teresa_init_params(self->params);
	teresa_params* params = (teresa_params*) self->params;

	int N = params->N;
	float FPU = params->FPU;
	teresa_tree* tree = params->tree;
	teresa_node root = tree->root;
	NODE_PL(root) = notme;	// Root node is "what was just played", i.e. by opponent
	
	state st;
	int t;
	for (t = 0; t < N; ++t) {
		teresa_node current = root;
		teresa_node child = NODE_NULL;
		state_copy(st0, &st);

		// Recurse into tree (think of next moves from what you played before)
		while (NODE_CHILD(current)) {
			if (NODE_UNEXPLORED_COUNT(current)) {
				child = teresa_select_best_child(tree, current, params, st.nextPlayer == me, FPU);
				if (!child) {
					move mv = teresa_extract_unexplored_move(tree, current);
					child = teresa_node_create_child(tree, current, &mv);
				}
			} else {
				child = teresa_select_best_child(tree, current, params, st.nextPlayer == me, -INFINITY);
			}
			current = child;
			chess_play_move(&st, &NODE_MV(current));
		}

		playout_result result;

		// Estimate result of node, expanding & playing thru if necessary
		if (chess_is_game_over(&st)) {
			
			// Leaf node; don't expand, just find out who won
			result.winner = state_winner(&st);

		} else {

			// Expansion (find things you never thought of before)
			int n = teresa_generate_unexplored_moves(&st, tree, current);
			assert(n);	// If not game over, there's gotta be a move we can play

			move mv = teresa_extract_unexplored_move(tree, current);
			current = teresa_node_create_child(tree, current, &mv);

			// Simulation (guessing what happens if you do certain things)
			chess_play_move(&st, &NODE_MV(current));
			chess_play_out(&st, &result);
		}

		// Back-propagation (remember what's learned)
		do {
			++NODE_VISITS(current);
			if (result.winner == me) {
				++NODE_WINS(current);
			}
			NODE_PWIN(current) = NAN;
			NODE_SQLG_VISITS(current) = NAN;
			NODE_RSQRT_VISITS(current) = NAN;

			current = NODE_PARENT(current);
		} while (current != NODE_NULL);
	}

	// Select most visited move (done thinking through all courses of action)
	teresa_node best_node = teresa_select_most_visited_child(tree, root);
	move best = NODE_MV(best_node);

	if (TERESA_DEBUG) {
		// wprintf(L"\nDecision heatmap\n");
		// teresa_old_print_heatmap(st0, root);

		wprintf(L"Win estimated at %.1f%%\n", node_pwin(tree, best_node)*100);
		wprintf(L"Confidence is %.1f%%\n", (float)NODE_VISITS(best_node)/NODE_VISITS(NODE_PARENT(best_node))*100);

		wprintf(L"This tree had %d out of %d nodes total (before move)\n", NODE_VISITS(root), teresa_node_count);
	}

	// TODO Uncomment once adapted to new node structure
	// if (me == BLACK) {
	// 	g2(root, "graph1.json", 8, 8);
	// } else {
	// 	g2(root, "graph3.json", 8, 8);
	// }

	// Resign if under win threshold :/
	if ((float)NODE_WINS(best_node) / NODE_VISITS(best_node) < TERESA_RESIGN_THRESHOLD) {
		teresa_node_destroy(tree, root);
		tree->root = NODE_NULL;
		*mv = MOVE_RESIGN;
		return chess_play_move(st0, mv);
	}

	// Destroy now useless children (forget everything unrelated to selected best move)
	teresa_destroy_all_children_except_one(tree, root, best_node);
	tree->root = root = best_node;

	if (TERESA_DEBUG) {
		wprintf(L"This tree has %d out of %d nodes total (after move)\n", NODE_VISITS(root), teresa_node_count);
	}

	*mv = best;
	return chess_play_move(st0, &best);
}

static void teresa_reset_all_trace_of_move(teresa_tree* tree, teresa_node nd, move* mv) {
	teresa_node child = NODE_CHILD(nd);
	teresa_node prev = NODE_NULL;
	teresa_node next = NODE_NULL;
	// Once mv found, reset whole branch
	while (child) {
		if (NODE_MV(child) == *mv) {
			next = NODE_SIBLING(child);
			NODE_WINS(child) = 0;
			NODE_VISITS(child) = 0;
			if (NODE_CHILD(child)) {
				teresa_destroy_all_children_except_one(tree, child, NODE_NULL);
			}
			break;
		}
		prev = child;
		child = NODE_SIBLING(child);
	}

	// Repair rest of tree after mass destruction
	if (next) {
		if (!prev) {
			NODE_CHILD(nd) = next;
		} else {
			NODE_SIBLING(prev) = next;
		}
	}

	// Recurse on each child
	child = NODE_CHILD(nd);
	while (child) {
		teresa_reset_all_trace_of_move(tree, child, mv);
		child = NODE_SIBLING(child);
	}
}

void teresa_observe(player* self, state* st, color opponent, move* opponent_mv) {
	st = st;	// @gcc pls dont warn kthx
	opponent = opponent;

	teresa_params* params = self->params;
	teresa_tree* tree = params->tree;
	if (!tree) return;
	teresa_node root = tree->root;

	if (TERESA_DEBUG) {
		wprintf(L"This tree had %d visits out of %d nodes total (before observing)\n", NODE_VISITS(root), teresa_node_count);
	}

	// Look for node with opponent_mv
	teresa_node child = NODE_CHILD(root);
	teresa_node found = NODE_NULL;
	while (child) {
		if (NODE_MV(child) == *opponent_mv) {
			found = child;
		}
		child = NODE_SIBLING(child);
	}

	if (found) {
		if (TERESA_DEBUG) {
			wprintf(L"Win estimated at %.1f%%\n", node_pwin(tree, found)*100);
			wprintf(L"Move is %.1f%% probable\n", (float)NODE_VISITS(found)/NODE_VISITS(root)*100);
			
			teresa_node expected = teresa_select_most_visited_child(tree, root);
			if (expected == found) {
				wprintf(L"This is the expected move");
			} else {
				wprintf(L"Expected move is ");
				move_print(&NODE_MV(expected));
			}
			wprintf(L" (%.1f%% win, %.1f%% confidence)\n", node_pwin(tree, expected)*100, (float)NODE_VISITS(expected)/NODE_VISITS(root)*100);
		}
		
		teresa_destroy_all_children_except_one(tree, root, found);
		tree->root = root = found;
	} else if (*opponent_mv == MOVE_RESIGN) {
		wprintf(L"I observed a resignation\n");
		teresa_node_destroy(tree, root);
		tree->root = NODE_NULL;
	} else {
		wprintf(L"Error: Teresa could not observe opponent move ");
		move_print(opponent_mv);
		wprintf(L"\n");
		wprintf(L"This is not normal and must be an untreated edge case.\n");
	}

	// Go through each child; once move opponent_mv found, delete the whole branch; then recurse on each child
	// Look at all children of node; find move opponent_mv, and delete the whole branch; then going through each child, repeat
	// teresa_old_reset_all_trace_of_move(root, opponent_mv);

	if (TERESA_DEBUG) {
		wprintf(L"This tree has %d visits out of %d nodes total (after observation)\n", NODE_VISITS(root), teresa_node_count);
	}
}

// Matching defines at top of file
#undef NODE_NULL
#undef NODE_PARENT
#undef NODE_SIBLING
#undef NODE_CHILD
#undef NODE_PL
#undef NODE_MV
#undef NODE_WINS
#undef NODE_VISITS
#undef NODE_PWIN
#undef NODE_SQLG_VISITS
#undef NODE_RSQRT_VISITS
#undef NODE_UNEXPLORED_MOVES
#undef NODE_UNEXPLORED_COUNT
