#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <wchar.h>

#include "players.h"
#include "utils.h"

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

void teresa_node_init(teresa_node* nd) {
	nd->parent = NULL;
	nd->sibling = NULL;
	nd->child = NULL;
	nd->mv = MOVE_PASS;
	nd->wins = 0;
	nd->visits = 0;
}

teresa_node* teresa_node_create() {
	void* mem = malloc(sizeof(teresa_node));
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
}

// Destroy a node and its children
void teresa_node_destroy(teresa_node* item) {
	if (item->child) {
		teresa_node_destroy_recursive(item->child);
	}
	free((void*) item);
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

		root->parent = NULL;
		root->sibling = NULL;
		root->child = NULL;
		root->mv = MOVE_PASS;
		root->wins = 0;
		root->visits = 0;
	}
}

// Pick a random item in arr that has given value
int pick_value_f(float* arr, int n, float val, int occurrences) {
	int j = RANDI(0, occurrences);
	for (int i = 0; i < n; ++i) {
		if (arr[i] == val && j-- == 0) {
			return i;
		}
	}
	return -1;
}

// Return child with highest UCB score
teresa_node* teresa_select_best_child(teresa_node* current, float C, bool friendly_turn) {
	float k = (current->visits == 0) ? 1 : C * sqrt(log(current->visits));

	float UCBs[NMOVES];

	int i = 0;
	float max_UCB = -INFINITY;
	int nmax_UCB = 0;
	teresa_node* selected_child = NULL;
	teresa_node* child = current->child;
	while (child) {
		if (friendly_turn) {
			UCBs[i] = (child->visits == 0) ? INFINITY : ((double)child->wins/child->visits) + k / sqrt(child->visits);
		} else {
			UCBs[i] = (child->visits == 0) ? INFINITY : 1 - ((double)child->wins/child->visits) + k / sqrt(child->visits);
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

// Return child with highest win probability
teresa_node* teresa_select_likeliest_child(teresa_node* current) {
	float probs[NMOVES];

	int i = 0;
	float max_prob = -INFINITY;
	int nmax_prob = 0;
	teresa_node* selected_child = NULL;
	teresa_node* child = current->child;
	while (child) {
		float prob = (float)child->wins/child->visits;
		probs[i] = prob;

		// Count max values
		if (prob == max_prob) {
			++nmax_prob;
		} else if (prob > max_prob) {
			max_prob = prob;
			nmax_prob = 1;
			selected_child = child;
		}

		++i;
		child = child->sibling;
	}

	if (nmax_prob == 1) {
		return selected_child;
	} else {
		int idx_max = pick_value_f(probs, i, max_prob, nmax_prob);
		assert(idx_max != -1);
		return teresa_node_sibling(current->child, idx_max);
	}
}

void teresa_print_heatmap(state* st, teresa_node* current, float C) {
	double UCBs[NMOVES];
	move mvs[NMOVES];

	float k = (current->visits == 0) ? 1 : C * sqrt(log(current->visits));

	int i = 0;
	teresa_node* child = current->child;
	while (child) {
		UCBs[i] = (child->visits == 0) ? INFINITY : ((double)child->wins/child->visits) + k / sqrt(child->visits);
		wprintf(L"%d\t%lf\n", i, UCBs[i]);
		mvs[i] = child->mv;

		++i;
		child = child->sibling;
	}

	go_print_heatmap(st, mvs, UCBs, i);
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
			k = PARAM_C * sqrt(log(nd->parent->visits));
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

// params.N, params.C must be defined
move_result teresa_play(player* self, state* st0, move* mv) {
	teresa_init_params(self->params);
	teresa_params* params = (teresa_params*) self->params;

	int N = params->N;
	float C = params->C;
	// teresa_pool* pool = params->pool;
	teresa_node* root = params->root;
	
	color me = st0->nextPlayer;
	color notme = (me == BLACK) ? WHITE : BLACK;

	state st;
	int t;
	int gameover_count = 0;
	for (t = 0; t < N; ++t) {
		teresa_node* current = root;
		state_copy(st0, &st);

		// Recurse into tree (think of next moves from what you played before)
		while (current->child) {
			current = teresa_select_best_child(current, C, st.nextPlayer == me);
			go_play_move(&st, &(current->mv));
		}

		if (go_is_game_over(&st)) {
			++gameover_count;
			// --t;	// HACK HACK HACK HACK
			// Also, is this preventing the loop from terminating? Consider case where tree only has losing moves

			// Propagate result up the tree (also HACK HACK HACK HACK)
			float score[3] = {0.0, 0.0, 0.0};
			state_score(&st, score, false);
			do {
				++current->visits;
				if (score[me] > score[notme]) {
					++current->wins;
				}
				current = current->parent;
			} while (current != NULL);

			continue;
		}

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

			if (prev_child) prev_child->sibling = child;
			if (i == 0) current->child = child;
			child->sibling = NULL;
			child->parent = current;
			child->child = NULL;
			child->mv = list[i];
			child->wins = 0;
			child->visits = 0;

			if (i == r) selected_child = child;
			prev_child = child;
		}

		// Simulation (guessing what happens if you do certain things)
		current = selected_child;
		go_play_move(&st, &(current->mv));
		playout_result result;
		go_play_out(&st, &result);

		// Back-propagation (remember what's learned)
		do {
			++current->visits;
			if (result.winner == me) {
				++current->wins;
			}
			current = current->parent;
		} while (current != NULL);
	}

	// Select best move (done thinking through all courses of action)
	teresa_node* best_node = teresa_select_best_child(root, PARAM_C, true);
	move best = best_node->mv;

	wprintf(L"\nDecision heatmap\n");
	teresa_print_heatmap(st0, root, C);

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
		teresa_destroy_all_children_except_one(root, child);
		params->root = root = child;
	} else {
		wprintf(L"Error: Teresa could not observe opponent move ");
		move_print(opponent_mv);
		wprintf(L"\n");
	}

	// Go through each child; once move opponent_mv found, delete the whole branch; then recurse on each child
	// Look at all children of node; find move opponent_mv, and delete the whole branch; then going through each child, repeat
	// teresa_reset_all_trace_of_move(root, opponent_mv);
}