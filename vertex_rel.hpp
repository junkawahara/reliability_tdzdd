#include "vconst_op.hpp"
#include "ToShiftedBDD.hpp"

/**
 * Computes vertex reliability for a given graph using BDD (Binary Decision Diagram) techniques.
 * 
 * This function transforms an edge-based BDD representation into an edge-vertex BDD representation
 * where both edges and vertices can fail independently. The algorithm creates a new variable
 * ordering that interleaves vertex and edge variables to efficiently represent the reliability
 * polynomial.
 * 
 * @param gv GlobalVariables containing graph, edge DD, and probability information
 * @return bddp representing the reliability polynomial as a BDD
 */
bddp computeVertexReliability(GlobalVariables& gv) {
    // Extract references to input data structures
    const tdzdd::Graph& graph = *(gv.graph);
    const tdzdd::DdStructure<2>& edge_dd = *(gv.edge_dd);
    std::vector<double>& edge_prob_list = gv.edge_prob_list;
    std::map<std::string, double>& vertex_prob_map = gv.vertex_prob_map;
    std::vector<double>& edge_vertex_prob_list = gv.edge_vertex_prob_list;
    
    const int n = graph.vertexSize();  // Number of vertices
    const int m = graph.edgeSize();    // Number of edges

    // Track which vertices have been encountered during edge traversal
    std::set<int> v_set;
    
    // Initialize mapping arrays for variable level assignment
    gv.v_list = new int[n + 1]; // Maps vertex number to its level in edge-vertex BDD
    gv.e_list = new int[m];     // Maps edge number (0-indexed) to its level in edge-vertex BDD
    std::vector<int> shift_vars(m + 1); // Maps original edge BDD level to new edge-vertex BDD level
    edge_vertex_prob_list.resize(n + m + 1, 0); // Maps edge-vertex BDD level to failure probability

    // Array to distinguish between vertex variables (1) and edge variables (0)
    int* is_vertex_list = new int[n + m + 1];

    // Variable level assignment: start from top level (m + n) and work downward
    // This creates an ordering where vertices appear before their incident edges
    int count = m + n;
    
    // Process each edge and assign levels to vertices and edges
    for (int i = 0; i < m; ++i) {
        const tdzdd::Graph::EdgeInfo& edge = graph.edgeInfo(i);
        int v1 = edge.v1;  // First endpoint of edge i
        int v2 = edge.v2;  // Second endpoint of edge i
        
        // Assign level to vertex v1 if not already processed
        if (v_set.count(v1) == 0) {
            v_set.insert(v1);
            gv.v_list[v1] = count;
            edge_vertex_prob_list[count] = vertex_prob_map[graph.vertexName(v1)];
            is_vertex_list[count] = 1; // Mark as vertex variable
            --count;
        }
        
        // Assign level to vertex v2 if not already processed
        if (v_set.count(v2) == 0) {
            v_set.insert(v2);
            gv.v_list[v2] = count;
            edge_vertex_prob_list[count] = vertex_prob_map[graph.vertexName(v2)];
            is_vertex_list[count] = 1; // Mark as vertex variable
            --count;
        }
        
        // Assign level to edge i
        gv.e_list[i] = count;
        shift_vars[m - i] = count; // Create mapping from original edge level to new level
        edge_vertex_prob_list[count] = edge_prob_list[i];
        is_vertex_list[count] = 0; // Mark as edge variable
        --count;
    }
    
    // Build incidence lists: for each vertex, list all incident edges
    // These lists are used in the BDD construction to enforce vertex-edge constraints
    int** inc_list = new int*[n + m + 1];      // Incidence list for each variable level
    int* inc_size_list = new int[n + m + 1];   // Size of each incidence list
    std::vector<std::vector<int> > inctemp_list;
    inctemp_list.resize(n + m + 1);
    
    // Populate incidence lists by examining each edge
    for (int i = 0; i < m; ++i) {
        const tdzdd::Graph::EdgeInfo& edge = graph.edgeInfo(i);
        int v1 = edge.v1;
        int v2 = edge.v2;
        // Add edge level to incidence lists of both endpoint vertices
        inctemp_list[gv.v_list[v1]].push_back(gv.e_list[i]);
        inctemp_list[gv.v_list[v2]].push_back(gv.e_list[i]);
    }

    // Convert temporary incidence lists to arrays and sort in descending order
    // Descending order ensures proper BDD variable ordering
    for (int i = 1; i < n + m + 1; ++i) {
        std::sort(inctemp_list[i].begin(), inctemp_list[i].end(), std::greater<int>());
        inc_size_list[i] = inctemp_list[i].size();
        inc_list[i] = new int[inctemp_list[i].size()];
        for (size_t j = 0; j < inctemp_list[i].size(); ++j) {
            inc_list[i][j] = inctemp_list[i][j];
        }
    }

#ifdef INPUT_CONFIRM_MODE
    // Debug output section: print all data structures for verification
    // This helps ensure correct variable assignment and incidence relationships
    std::cout << "=== INPUT CONFIRMATION ===\n";
    
    std::cout << "v_list:\n";
    for (int v : v_set) {
        std::cout << "  v[" << v << "] = " << gv.v_list[v] << "\n";
    }
    
    std::cout << "e_list:\n";
    for (int i = 0; i < m; ++i) {
        std::cout << "  e[" << i << "] = " << gv.e_list[i] << "\n";
    }

    std::cout << "shift_vars:\n";
    for (int i = 1; i <= m; ++i) {
        std::cout << "  shift_vars[" << i << "] = " << shift_vars[i] << "\n";
    }
    
    std::cout << "is_vertex_list:\n";
    for (int i = 1; i <= n + m; ++i) {
        std::cout << "  is_vertex[" << i << "] = " << is_vertex_list[i] << "\n";
    }
    
    std::cout << "inc_list and inc_size_list:\n";
    for (int i = 1; i <= n + m; ++i) {
        std::cout << "  inc_size[" << i << "] = " << inc_size_list[i] << ", inc_list[" << i << "] = [";
        for (int j = 0; j < inc_size_list[i]; ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << inc_list[i][j];
        }
        std::cout << "]\n";
    }

    std::cout << "edge_vertex_prob_list:\n";
    for (int i = 1; i <= n + m; ++i) {
        std::cout << "  edge_vertex_prob[" << i << "] = " << edge_vertex_prob_list[i] << "\n";
    }

    std::cout << "=== END INPUT CONFIRMATION ===\n";
    exit(0);
#endif

    // Initialize BDD library with specified memory constraints
    BDD_Init(256ll, BDD_MaxNode);

    // Create BDD variables for all levels (vertices + edges + dummy level 0)
    for (int i = 0; i < n + m + 1; ++i) {
        BDD_NewVar();
    }

    // Transform the original edge-only BDD to the new edge-vertex variable ordering
    gv.shifted_edge_dd = edge_dd.evaluate(tdzdd::ToShiftedBDD(shift_vars));

    // Build the final edge-vertex BDD that represents the reliability polynomial
    // This BDD encodes all valid configurations where the graph remains connected
    // considering both edge and vertex failures
    bddp g = build_ev_bdd(n + m, gv.shifted_edge_dd.GetID(), is_vertex_list, inc_list, inc_size_list);
    return g;
}
