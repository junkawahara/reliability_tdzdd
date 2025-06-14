#ifdef USE_PCH
#include "pch.hpp"
#endif
#include <cassert>
#include <map>
#include <utility>
#include "SAPPOROBDD/bddc.cc"

// Define operation codes for BDD manipulation
//#define BC_VZDD       21
#define BC_EVBDD      22  // Operation code for Edge-Valued BDD
#define BC_INSERTVALL 23  // Operation code for inserting multiple vertices

// Cache for storing results of insertVAll function to avoid redundant computation
// Key: pair of (base BDD node pointer, dependency list pointer), Value: resulting BDD node pointer
static std::map<std::pair<bddp, const int*>, bddp> insert_vall_cache;

/**
 * Inserts multiple vertices into a BDD based on a dependency list.
 * 
 * This function inserts vertices represented in dep_list into the base_f BDD.
 * The dependency list must be sorted in descending order.
 * 
 * @param dep_list      Array of vertex indices to be inserted, sorted in descending order
 * @param dep_list_size Size of the dependency list
 * @param base_f        Base BDD into which vertices will be inserted
 * @return              Resulting BDD after insertion of all vertices
 */
bddp insertVAll(const int* dep_list, int dep_list_size, bddp base_f)
{
    struct B_NodeTable *fp;
    bddvar flev, fvar;
    bddp f0, f1, r0, r1, h;

    // Handle terminal cases: if base_f is a terminal node (true/false), no insertion needed
    if (base_f == bddfalse || base_f == bddtrue) {
        return base_f;
    }

    // Get node information for base_f
    fp = B_NP(base_f);
    fvar = B_VAR_NP(fp);       // Variable ID of the node
    flev = Var[fvar].lev;      // Level of the variable in the BDD

    // Skip vertices in dep_list that are above the current node's level
    // (Since dep_list is sorted in descending order, we can just skip them)
    while (dep_list_size > 0 && static_cast<unsigned int>(dep_list[0]) > flev) {
        --dep_list_size;
        ++dep_list;
    }

    // If no more vertices to insert, return the current BDD
    if (dep_list_size == 0) {
        B_RFC_INC_NP(fp);  // Increment reference count before returning
        return base_f;
    } else {
        // Check cache to avoid redundant computation
        if (!B_RFC_ONE_NP(fp)) {
            auto cache_key = std::make_pair(base_f, dep_list);
            auto cache_it = insert_vall_cache.find(cache_key);
            if (cache_it != insert_vall_cache.end()) {
                // Cache hit - return the cached result
                h = cache_it->second;
                if(!B_CST(h) && h != bddnull) { fp = B_NP(h); B_RFC_INC_NP(fp); }
                return h;
            }
        }

        // Recursive computation based on the relationship between current vertex level and BDD node level
        if (static_cast<unsigned int>(dep_list[0]) == flev) {
            // If the top vertex in dep_list matches the current node's level,
            // insert the remaining vertices into the 0-edge (low) child
            f0 = B_GET_BDDP(fp->f0);
            h = insertVAll(dep_list + 1, dep_list_size - 1, f0);
        } else {
            // Otherwise, insert all vertices into both children
            f0 = B_GET_BDDP(fp->f0);
            f1 = B_GET_BDDP(fp->f1);
            r0 = insertVAll(dep_list, dep_list_size, f0);
            r1 = insertVAll(dep_list, dep_list_size, f1);
            
            // Verify that the resulting nodes are at lower levels
            assert(flev > bddlevofvar(bddtop(r0)) && flev > bddlevofvar(bddtop(r1)));
            
            // Construct a new node with the same variable and new children
            h = getbddp(fvar, r0, r1);
        }
        
        // Handle negation if needed
        if (B_NEG(base_f)) {
            h = B_NOT(h);
        }

        // Store result in cache for future lookup
        if (!B_RFC_ONE_NP(B_NP(base_f)) && h != bddnull) {
            auto cache_key = std::make_pair(base_f, dep_list);
            insert_vall_cache[cache_key] = h;
        }

        return h;
    }
}

/**
 * Builds an Edge-Vertex BDD (EVBDD) from an edge BDD.
 * 
 * This function recursively constructs an EVBDD by traversing the input BDD
 * and applying vertex constraints according to the provided vertex and incidence lists.
 * 
 * @param level          Current level being processed
 * @param f              BDD node being processed
 * @param is_vertex_list Array indicating which levels correspond to vertices (1) vs edges (0)
 * @param inc_list       Incidence list for each vertex (which edges are incident to each vertex)
 * @param inc_size_list  Size of each incidence list
 * @return               The resulting EVBDD
 */
bddp build_ev_bdd(int level, bddp f, const int* is_vertex_list,
                  const int* const* inc_list, const int* inc_size_list)
{
    struct B_NodeTable *fp;
    struct B_CacheTable *cachep;
    bddp f0, f1, h0, h1, h, key;
    bddvar flev;

    // Base cases: reached bottom level or terminal node
    if (level == 0 || f == bddfalse || f == bddtrue) {
        return f;
    }

    // Check cache for previously computed result
    fp = B_NP(f);
    if (B_RFC_ONE_NP(fp)) {
        key = bddnull;  // Don't cache if reference count is 1
    } else {
        key = B_CACHEKEY(BC_EVBDD, f, (bddp)level);
        cachep = Cache + key;
        if (cachep->op == BC_EVBDD &&
            f == B_GET_BDDP(cachep->f) &&
            (bddp)level == B_GET_BDDP(cachep->g)) {
            // Cache hit - return the cached result
            h = B_GET_BDDP(cachep->h);
            if(!B_CST(h) && h != bddnull) { fp = B_NP(h); B_RFC_INC_NP(fp); }
            return h;
        }
    }

    // Get the level of the current BDD node
    flev = Var[B_VAR_NP(fp)].lev;
    
    // Skip non-vertex levels above the current BDD node
    while (is_vertex_list[level] == 0 && static_cast<unsigned int>(level) > flev) {
        --level;
    }

    // Process differently depending on whether current level represents a vertex or not
    if (is_vertex_list[level] != 0) {
        // For vertex levels, apply vertex constraints
        h1 = build_ev_bdd(level - 1, f, is_vertex_list, inc_list, inc_size_list);
        h0 = insertVAll(inc_list[level], inc_size_list[level], h1);
        
        // Verify that the resulting nodes are at lower levels
        assert(static_cast<unsigned int>(level) > Var[bddtop(h0)].lev && static_cast<unsigned int>(level) > Var[bddtop(h1)].lev);
        
        // Construct the node for the current level
        h = getbddp(bddvaroflev(level), h0, h1);
    } else {
        // For non-vertex levels, recursively process both children
        f0 = B_GET_BDDP(fp->f0);
        f1 = B_GET_BDDP(fp->f1);
        h0 = build_ev_bdd(level - 1, f0, is_vertex_list, inc_list, inc_size_list);
        h1 = build_ev_bdd(level - 1, f1, is_vertex_list, inc_list, inc_size_list);
        
        // Verify that the resulting nodes are at lower levels
        assert(static_cast<unsigned int>(level) > Var[bddtop(h0)].lev && static_cast<unsigned int>(level) > Var[bddtop(h1)].lev);
        
        // Construct the node for the current level, preserving negation from original node
        h = getbddp(bddvaroflev(level), h0, h1);
        if (B_NEG(f)) {
            h = B_NOT(h);
        }
    }
    
    // Store the result in cache
    if (key != bddnull && h != bddnull) {
        cachep = Cache + key;
        cachep->op = BC_EVBDD;
        B_SET_BDDP(cachep->f, f);
        B_SET_BDDP(cachep->g, (bddp)level);
        B_SET_BDDP(cachep->h, h);
    }
    return h;
}
