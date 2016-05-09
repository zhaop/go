#ifndef PLAYERS_TERESA_H
#define PLAYERS_TERESA_H

#define TERESA_MAX_NODES 200000*(COUNT)
#define TERESA_RESIGN_THRESHOLD 0.01
#define TERESA_DEBUG 1

typedef uint32_t teresa_node;

// Simultaneously holds decision tree & flat "free" tree (only siblings)
// Node 0 is reserved for NULL node (this means tree holds in fact TERESA_MAX_NODES-1 values)
// In free tree, only sibling contains valid values
struct teresa_tree;
typedef struct teresa_tree {
	teresa_node root;
	teresa_node freeroot;
	teresa_node parent[TERESA_MAX_NODES];
	teresa_node sibling[TERESA_MAX_NODES];
	teresa_node child[TERESA_MAX_NODES];
	color pl[TERESA_MAX_NODES];
	move mv[TERESA_MAX_NODES];
	uint32_t wins[TERESA_MAX_NODES];
	uint32_t visits[TERESA_MAX_NODES];
	float pwin[TERESA_MAX_NODES];
	float sqlg_visits[TERESA_MAX_NODES];
	float rsqrt_visits[TERESA_MAX_NODES];
	move* unexplored_moves[TERESA_MAX_NODES];
	uint16_t unexplored_count[TERESA_MAX_NODES];
} teresa_tree;

struct teresa_old_node;
typedef struct teresa_old_node {
	struct teresa_old_node* parent;	// 8b
	struct teresa_old_node* sibling;// 8b
	struct teresa_old_node* child;	// 8b
	float pwin;					// 4b
	float sqlg_visits;			// 4b	// BUG ONLY READ USING node_sqlg_visits(&node)
	float rsqrt_visits;			// 4b	// BUG ONLY READ USING node_rsqrt_visits(&node)
	uint32_t wins;				// 4b	// BUG Must also set pwin NAN!
	uint32_t visits;			// 4b	// BUG Must also set pwin, sqlg_visits, rsqrt_visits NAN!
	move mv;					// 2b
	color pl;					// 1b
	move* unexplored_moves;		// 8b
	uint16_t unexplored_count;	// 2b
								// = 57b => 64b
} teresa_old_node;

typedef struct {
	int N;
	float C;
	float FPU;
	struct teresa_tree* tree;
	struct teresa_old_node* old_root;
} teresa_params;

move_result teresa_play(player*, state*, move*);
void teresa_observe(player*, state*, color, move*);

void g(teresa_tree*, teresa_node);
void g2(teresa_tree*, teresa_node, const char*, int, int);

#endif