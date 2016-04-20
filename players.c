#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <wchar.h>

#include "players.h"
#include "utils.h"

static unsigned int teresa_node_count = 0;

// params is ignored for human play
move_result human_play(player* self, state* st, move* mv) {
	self = self;	// @gcc pls dont warn kthx
	while (1) {
		wprintf(L"Your move: %lc ", color_char(st->nextPlayer));
		
		char mv_in[2];	// TODO DANGER BUFFER OVURFLURW
		scanf("%s", mv_in);

		if (!move_parse(mv, mv_in)) {
			wprintf(L"Invalid input\n");
		} else if (!go_is_move_legal(st, mv)) {
			wprintf(L"Move is illegal\n");
		} else {
			move_result result = go_play_move(st, mv);

			if (result != SUCCESS) {
				wprintf(L"Unsuccessful move: ");
				go_print_move_result(result);
			} else {
				return result;
			}
		}
	}
}

// Have bot play one move given current state
move_result randy_play(player* self, state* st, move* mv) {
	self = self;
	move move_list[NMOVES];
	return go_play_random_move(st, mv, move_list);
}

move_result karl_play(player* self, state* st, move* mv) {
	int N = ((karl_params*) self->params)->N;

	state test_st;

	move legal_moves[NMOVES];
	int num_moves = go_get_legal_moves(st, legal_moves);

	if (num_moves == 1) {
		*mv = legal_moves[0];
		return go_play_move(st, mv);
	}

	color me = st->nextPlayer;
	color notme = (me == BLACK) ? WHITE : BLACK;

	int win[NMOVES];
	int lose[NMOVES];
	double pwin[NMOVES];

	int i;
	for (i = 0; i < num_moves; ++i) {
		win[i] = lose[i] = 0;
		pwin[i] = 0.0;
	}

	// Do playouts
	for (int i = 0; i < N; ++i) {
		state_copy(st, &test_st);

		int starting_move_idx = RANDI(0, num_moves);
		move starting_move = legal_moves[starting_move_idx];
		go_play_move(&test_st, &starting_move);

		playout_result result;
		go_play_out(&test_st, &result);

		if (result.winner == me) {
			++win[starting_move_idx];
		} else if (result.winner == notme) {
			++lose[starting_move_idx];
		}
	}

	// Calculate chances
	double best_pwin = 0;
	move best_pwin_move;
	for (i = 0; i < num_moves; ++i) {

		if (win[i] + lose[i] == 0) {
			pwin[i] = 0;
		} else {
			pwin[i] = (double) win[i] / (win[i] + lose[i]);
		}

		if (pwin[i] > best_pwin) {
			best_pwin = pwin[i];
			best_pwin_move = legal_moves[i];
		};
	}

	wprintf(L"\nDecision heatmap\n");
	go_print_heatmap(st, legal_moves, pwin, num_moves);

	wprintf(L"Going with ");
	move_print(&best_pwin_move);
	wprintf(L" at %.1f%%\n", best_pwin*100);

	*mv = best_pwin_move;
	return go_play_move(st, mv);
}

static inline void teresa_node_init(teresa_node* nd) {
	nd->parent = NULL;
	nd->sibling = NULL;
	nd->child = NULL;
	nd->pl = NEUTRAL;
	nd->mv = MOVE_PASS;
	nd->wins = 0;
	nd->visits = 0;
	nd->pwin = NAN;
	nd->sqlg_visits = NAN;
	nd->rsqrt_visits = NAN;
}

static inline float node_pwin(teresa_node* nd) {
	return isnan(nd->pwin) ? (nd->pwin = (float)nd->wins/nd->visits) : nd->pwin;
}

static inline float node_sqlg_visits(teresa_node* nd) {
	return isnan(nd->sqlg_visits) ? (nd->sqlg_visits = sqrt(log((float)nd->visits))) : nd->sqlg_visits;
}

static inline float node_rsqrt_visits(teresa_node* nd) {
	return isnan(nd->rsqrt_visits) ? (nd->rsqrt_visits = 1 / sqrt((float)nd->visits)) : nd->rsqrt_visits;
}

teresa_node* teresa_node_create() {
	void* mem = malloc(sizeof(teresa_node));
	++teresa_node_count;
	return (teresa_node*) mem;
}

// Destroys a node's siblings and children
void teresa_node_destroy_recursive(teresa_node* item) {
	if (item->child) {
		teresa_node_destroy_recursive(item->child);
	}
	if (item->sibling) {
		teresa_node_destroy_recursive(item->sibling);
	}
	free((void*) item);
	--teresa_node_count;
}

// Destroy a node and its children
void teresa_node_destroy(teresa_node* item) {
	if (item->child) {
		teresa_node_destroy_recursive(item->child);
	}
	free((void*) item);
	--teresa_node_count;
}

teresa_node* teresa_node_sibling(teresa_node* node, int idx) {
	for (int i = 0; i < idx; ++i) {
		node = node->sibling;
		if (!node) {
			return NULL;
		}
	}
	return node;
}

static void teresa_init_params(void* params) {
	teresa_pool* pool = ((teresa_params*) params)->pool;
	if (!pool) {
		pool = malloc(sizeof(teresa_pool));
		((teresa_params*) params)->pool = pool;
	}
	teresa_node* root = ((teresa_params*) params)->root;
	if (!root) {
		root = teresa_node_create();
		((teresa_params*) params)->root = root;
		teresa_node_init(root);
	}
}

// Return child with highest UCB score
teresa_node* teresa_select_best_child(teresa_node* current, float C, bool friendly_turn) {
	float k = (current->visits == 0) ? 1.0 : C * node_sqlg_visits(current);

	float UCBs[NMOVES];

	int i = 0;
	float max_UCB = -INFINITY;
	int nmax_UCB = 0;
	teresa_node* selected_child = NULL;
	teresa_node* child = current->child;
	while (child) {
		if (child->visits) {
			if (friendly_turn) {
				UCBs[i] = node_pwin(child) + k * node_rsqrt_visits(child);
			} else {
				UCBs[i] = 1 - node_pwin(child) + k * node_rsqrt_visits(child);
			}
		} else {
			UCBs[i] = INFINITY;
		}

		// Count max values
		if (UCBs[i] == max_UCB) {
			++nmax_UCB;
		} else if (UCBs[i] > max_UCB) {
			max_UCB = UCBs[i];
			nmax_UCB = 1;
			selected_child = child;
		}

		++i;
		child = child->sibling;
	}

	if (nmax_UCB == 1) {
		return selected_child;
	} else {
		int idx_max = pick_value_f(UCBs, i, max_UCB, nmax_UCB);
		assert(idx_max != -1);
		return teresa_node_sibling(current->child, idx_max);
	}
}

// Return most visited child (the one we're most certain of?)
teresa_node* teresa_select_most_visited_child(teresa_node* current) {
	float visits[NMOVES];	// float because pick_value_f only takes floats (overkill?)

	int i = 0;
	float max_visits = 0;
	int nmax = 0;
	teresa_node* selected_child = NULL;
	teresa_node* child = current->child;
	while (child) {
		float visit = (float) child->visits;
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
		child = child->sibling;
	}

	if (nmax == 1) {
		return selected_child;
	} else {
		int idx_max = pick_value_f(visits, i, max_visits, nmax);
		assert(idx_max != -1);
		return teresa_node_sibling(current->child, idx_max);
	}
}

void teresa_print_heatmap(state* st, teresa_node* current) {
	double values[NMOVES];
	move mvs[NMOVES];

	int i = 0;
	teresa_node* child = current->child;
	while (child) {
		values[i] = node_pwin(child);
		mvs[i] = child->mv;

		++i;
		child = child->sibling;
	}

	go_print_heatmap(st, mvs, values, i);
}

void teresa_destroy_all_children_except_one(teresa_node* parent, teresa_node* keep) {
	teresa_node* next = NULL;
	teresa_node* current = parent->child;
	do {
		next = current->sibling;
		if (current == keep) {
			current->sibling = NULL;
			current->parent = NULL;
		} else {
			teresa_node_destroy(current);
		}
		current = next;
	} while (current);
}

#define PARAM_C 0.5

void pshort(teresa_node* nd) {
	wprintf(L"{");
	
	if (nd) {
		move_print(&nd->mv);

		float k = 1;
		if (nd->parent && nd->parent->visits != 0) {
			k = PARAM_C * node_sqlg_visits(nd->parent);
		}
		wprintf(L", %d/%d; %.3f, %.3f", nd->wins, nd->visits,
			((double)nd->wins/nd->visits) + k / sqrt(nd->visits), 1 - ((double)nd->wins/nd->visits) + k / sqrt(nd->visits));
	}
	wprintf(L"}");

	if (nd && nd->parent && nd == teresa_select_best_child(nd->parent, PARAM_C, true)) {
		wprintf(L"*");
	}
	if (nd && nd->parent && nd == teresa_select_best_child(nd->parent, PARAM_C, false)) {
		wprintf(L"^");
	}
}

void p(teresa_node* nd) {
	wprintf(L"teresa_node (%p) {\n", nd);
	wprintf(L"  parent:\n    ");
	pshort(nd->parent);
	
	wprintf(L"\n  self:\n    ");
	pshort(nd);
	
	wprintf(L"\n  siblings:");
	teresa_node* current = nd;
	while (current->sibling) {
		wprintf(L"\n    ");
		pshort(current->sibling);
		current = current->sibling;
	}
	
	wprintf(L"\n  child:\n    ");
	pshort(nd->child);

	wprintf(L"\n}\n");
}

int int_desc_cmp(const void* a, const void* b) {
	return *((int*)b) - *((int*)a);
}

void graph_tree(FILE* f, teresa_node* node, int depth, int cutoff) {
	if (!node) return;
	
	wchar_t mv_str[3];
	move_sprint(mv_str, &(node->mv));

	char color_c = (node->pl == BLACK) ? 'b' : (node->pl == WHITE ? 'w' : 'n');

	fprintf(f, "{\"id\":\"%p\",\"player\":\"%c\",\"move\":\"%ls\",\"visits\":%d,\"wins\":%d", node, color_c, mv_str, node->visits, node->wins);

	if (depth > 0 && node->child) {
		--depth;
		teresa_node* child;

		// Find highest visits first
		int node_visits[NMOVES];
		int i = 0;
		child = node->child;
		while (child) {
			node_visits[i] = child->visits;
			++i;
			child = child->sibling;
		}
		qsort(node_visits, i, sizeof(int), int_desc_cmp);
		uint thresh = max(node_visits[min(cutoff, i)-1], 40);
		
		if (i) {
			child = node->child;
			bool nothing_printed = true;
			while (child) {
				if (child->visits >= thresh) {
					if (nothing_printed) {
						fprintf(f, ",\"children\":[\n");
						nothing_printed = false;
					} else {
						fprintf(f, ",\n");
					}
					graph_tree(f, child, depth, cutoff);
				}
				child = child->sibling;
			}
			if (!nothing_printed) {
				fprintf(f, "]");
			}
		}
	}

	fprintf(f, "}");
}

void g(teresa_node* root) {
	g2(root, "graph1.json", 7, 7);
}

void g2(teresa_node* root, const char* path, int depth, int thresh) {

	if (!root) {
		wprintf(L"root is NULL\n");
		return;
	}

	const char fmode = 'w';
	
	FILE* f = fopen(path, &fmode);
	graph_tree(f, root, depth, thresh);
	
	fclose(f);
}

// params.N, params.C must be defined
move_result teresa_play(player* self, state* st0, move* mv) {
	color me = st0->nextPlayer;
	color notme = (me == BLACK) ? WHITE : BLACK;

	teresa_init_params(self->params);
	teresa_params* params = (teresa_params*) self->params;

	int N = params->N;
	float C = params->C;
	// teresa_pool* pool = params->pool;
	teresa_node* root = params->root;
	root->pl = notme;	// Root node is "what was just played", i.e. by opponent
	
	state st;
	int t;
	for (t = 0; t < N; ++t) {
		teresa_node* current = root;
		state_copy(st0, &st);

		// Recurse into tree (think of next moves from what you played before)
		while (current->child) {
			current = teresa_select_best_child(current, C, st.nextPlayer == me);
			go_play_move(&st, &(current->mv));
		}

		// Estimate result of node, expanding & playing thru if necessary
		playout_result result;
		if (go_is_game_over(&st)) {
			
			// Leaf node; don't expand, just find out who won
			result.winner = state_winner(&st);

		} else {

			// Expansion (find things you never thought of before)
			move list[NMOVES];
			int n = go_get_reasonable_moves(&st, list);
			int r = RANDI(0, n);
			teresa_node* child = NULL;
			teresa_node* prev_child = NULL;
			teresa_node* selected_child = NULL;
			for (int i = 0; i < n; ++i) {

				child = teresa_node_create();
				assert(child);

				teresa_node_init(child);

				if (prev_child) prev_child->sibling = child;
				if (i == 0) current->child = child;
				child->parent = current;
				child->pl = color_opponent(current->pl);
				child->mv = list[i];

				if (i == r) selected_child = child;
				prev_child = child;
			}

			// Simulation (guessing what happens if you do certain things)
			current = selected_child;
			go_play_move(&st, &(current->mv));
			go_play_out(&st, &result);
		}

		// Back-propagation (remember what's learned)
		do {
			++current->visits;
			if (result.winner == me) {
				++current->wins;
			}
			current->pwin = NAN;
			current->sqlg_visits = NAN;
			current->rsqrt_visits = NAN;

			current = current->parent;
		} while (current != NULL);
	}

	// Select most visited move (done thinking through all courses of action)
	teresa_node* best_node = teresa_select_most_visited_child(root);
	move best = best_node->mv;

	wprintf(L"\nDecision heatmap\n");
	teresa_print_heatmap(st0, root);

	if (me == BLACK) {
		g2(root, "graph1.json", 8, 8);
	} else {
		g2(root, "graph3.json", 8, 8);
	}

	// Resign if under win threshold :/
	if ((float)best_node->wins / best_node->visits < TERESA_RESIGN_THRESHOLD) {
		teresa_node_destroy(root);
		params->root = NULL;
		*mv = MOVE_RESIGN;
		return go_play_move(st0, mv);
	}

	// Destroy now useless children (forget everything unrelated to selected best move)
	teresa_destroy_all_children_except_one(root, best_node);
	params->root = root = best_node;

	*mv = best;
	return go_play_move(st0, &best);
}

void teresa_reset_all_trace_of_move(teresa_node* node, move* mv) {
	teresa_node* child = node->child;
	teresa_node* prev = NULL;
	teresa_node* next = NULL;
	// Once mv found, reset whole branch
	while (child) {
		if (child->mv == *mv) {
			next = child->sibling;
			child->wins = 0;
			child->visits = 0;
			if (child->child) {
				teresa_destroy_all_children_except_one(child, NULL);
			}
			break;
		}
		prev = child;
		child = child->sibling;
	}

	// Repair rest of tree after mass destruction
	if (next) {
		if (!prev) {
			node->child = next;
		} else {
			prev->sibling = next;
		}
	}

	// Recurse on each child
	child = node->child;
	while (child) {
		teresa_reset_all_trace_of_move(child, mv);
		child = child->sibling;
	}
}

void teresa_observe(player* self, state* st, color opponent, move* opponent_mv) {
	st = st;	// @gcc pls dont warn kthx
	opponent = opponent;

	teresa_params* params = self->params;
	teresa_node* root = params->root;
	if (!root) return;

	// Look for node with opponent_mv
	teresa_node* child = root->child;
	teresa_node* found = NULL;
	while (child) {
		if (child->mv == *opponent_mv) {
			found = child;
		}
		child = child->sibling;
	}

	if (found) {
		teresa_destroy_all_children_except_one(root, found);
		params->root = root = found;
	} else if (*opponent_mv == MOVE_PASS) {
		wprintf(L"I observed an unexpected pass, which confuses me\n");
		teresa_node_destroy(root);
		params->root = NULL;
	} else if (*opponent_mv == MOVE_RESIGN) {
		wprintf(L"I observed a resignation\n");
		teresa_node_destroy(root);
		params->root = NULL;
	} else {
		wprintf(L"Error: Teresa could not observe opponent move ");
		move_print(opponent_mv);
		wprintf(L"\n");
		wprintf(L"This is not normal and must be an untreated edge case.\n");
	}

	// Go through each child; once move opponent_mv found, delete the whole branch; then recurse on each child
	// Look at all children of node; find move opponent_mv, and delete the whole branch; then going through each child, repeat
	// teresa_reset_all_trace_of_move(root, opponent_mv);
}