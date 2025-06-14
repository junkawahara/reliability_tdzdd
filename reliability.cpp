/*
 * Reliability: Computing the network reliability with TdZdd
 * by Jun Kawahara
 * Copyright (c) 2017 -- 2025 Jun Kawahara
 *
 * This program is a subset of graphillion_example at
 * https://github.com/kunisura/TdZdd/tree/master/apps/graphillion
 * written by Hiroaki Iwashita.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef USE_PCH
#include "pch.hpp"
#else
#include <climits>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <set>
#include <cassert>

#include "tdzdd/DdSpecOp.hpp"
#include "tdzdd/DdStructure.hpp"
#include "tdzdd/DdEval.hpp"
#include "tdzdd/util/Graph.hpp"
#include "tdzdd/spec/FrontierBasedSearch.hpp"
#include "tdzdd/spec/SapporoBdd.hpp"
#endif

class GlobalVariables {
public:
    std::vector<double> edge_prob_list;
    std::map<std::string, double> vertex_prob_map;
    std::vector<double> edge_vertex_prob_list;
    int* v_list;
    int* e_list;
    const tdzdd::Graph* graph;
    const tdzdd::DdStructure<2>* edge_dd;
    BDD shifted_edge_dd;
};

#include "vertex_rel.hpp"
#include "alg_k.hpp"

//#include "FrontierBasedSearch.hpp"
//#include "Graph.hpp"

using namespace tdzdd;

std::string options[][2] = { //
        {"a", "Read <graph_file> as an adjacency list"}, //
        {"allrel", "Compute all terminal reliability (ignoring <vertex_group_file>)"}, //
        {"count", "Report the number of solutions"}, //
        {"graph", "Dump input graph to STDOUT in DOT format"}, //
        {"reduce", "Reduce result BDD"}, //
        {"solutions <n>", "Dump at most <n> solutions to STDOUT in DOT format"}, //
        {"zdd", "Dump result ZDD to STDOUT in DOT format"}, //
        {"export", "Dump result ZDD to STDOUT"},
        {"vertex", "Compute the reliability with imperfect vertices"},
        {"alg_k", "Run alg_k"},
        {"quiet", "Suppress output and only show OK/NG for vertex_dd == h comparison"}}; //

std::map<std::string,bool> opt;
std::map<std::string,int> optNum;
std::map<std::string,std::string> optStr;

class ProbEval: public tdzdd::DdEval<ProbEval,double> {
private:
    std::vector<double> prob_list_;
    
public:
    ProbEval(const std::vector<double>& edge_prob_list) : prob_list_(edge_prob_list) { }
    
    void evalTerminal(double& p, bool one) const {
        p = one ? 1.0 : 0.0;
    }

    void evalNode(double& p, int level, DdValues<double, 2> const& values) const {
        double pc = prob_list_[level];
        p = values.get(0) * (1 - pc) + values.get(1) * pc;
    }
};

void usage(char const* cmd) {
    std::cerr << "usage: " << cmd
                << " [ <option>... ] [ <graph_file> [ <vertex_group_file> [ <prob_file> ]]]\n";
    std::cerr << "options\n";
    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        std::cerr << "  -" << options[i][0];
        for (unsigned j = options[i][0].length(); j < 10; ++j) {
            std::cerr << " ";
        }
        std::cerr << ": " << options[i][1] << "\n";
    }
}

class EdgeDecorator {
    int const n;
    std::set<int> const& levels;

public:
    EdgeDecorator(int n, std::set<int> const& levels) :
            n(n), levels(levels) {
    }

    std::string operator()(Graph::EdgeNumber a) const {
        return levels.count(n - a) ?
                "[style=bold]" : "[style=dotted,color=gray]";
    }
};

void parse_graph_file(const std::string& filename, tdzdd::Graph& graph, std::vector<double>& edge_prob_list) {
    std::ifstream ifs(filename);
    std::string line;
    
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string token1, token2;
        double prob;
        
        if (iss >> token1 >> token2) {
            graph.addEdge(token1, token2);
            
            if (iss >> prob) {
                edge_prob_list.push_back(prob);
            }
        } else {
            throw std::runtime_error("ERROR: Invalid line in graph file: " + line);
        }
    }
    graph.update();
}

void parse_vertex_prob_file(const std::string& filename, std::map<std::string, double>& vertex_prob_map) {
    std::ifstream ifs(filename.c_str());
    if (!ifs) {
        throw std::runtime_error("ERROR: Cannot open vertex probability file: " + filename);
    }
    
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        
        // Replace comma with space for uniform processing
        for (size_t i = 0; i < line.length(); ++i) {
            if (line[i] == ',') {
                line[i] = ' ';
            }
        }
        
        std::istringstream iss(line);
        std::string vertex_name;
        double prob;
        
        if (iss >> vertex_name >> prob) {
            vertex_prob_map[vertex_name] = prob;
        }
    }
}

int main(int argc, char *argv[]) {

    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        opt[options[i][0]] = false;
    }

    std::string graphFileName;
    std::string termFileName;
    std::string edgeProbFileName;
    std::string vertexProbFileName;

    try {
        for (int i = 1; i < argc; ++i) {
            std::string s = argv[i];
            if (s[0] == '-') {
                // Remove hyphen(s)
                s = s.substr(1);
                
                // Handle double hyphen format (--option)
                if (s[0] == '-') {
                    s = s.substr(1);
                }

                // Handle --xxx=yyy format
                size_t pos = s.find('=');
                if (pos != std::string::npos) {
                    std::string key = s.substr(0, pos);
                    std::string value = s.substr(pos + 1);
                    optStr[key] = value;
                    continue;
                } else if (opt.count(s)) {
                    // Handle --xxx format (without =)
                    opt[s] = true;
                    continue;
                }

                if (opt.count(s)) {
                    opt[s] = true;
                }
                else if (i + 1 < argc && opt.count(s + " <n>")) {
                    opt[s] = true;
                    optNum[s] = std::atoi(argv[++i]);
                }
                else if (i + 1 < argc && opt.count(s + " " + argv[i + 1])) {
                    opt[s] = true;
                    optStr[s] = argv[++i];
                }
                else {
                    throw std::exception();
                }
            }
            else if (graphFileName.empty()) {
                graphFileName = s;
            }
            else if (termFileName.empty()) {
                termFileName = s;
            }
            else if (edgeProbFileName.empty()) {
                edgeProbFileName = s;
            }
            else {
                throw std::exception();
            }
        }
        if (graphFileName.empty()) { // show usage
            throw std::exception();
        }
    }
    catch (std::exception& e) {
        usage(argv[0]);
        return 1;
    }

    if (!opt["quiet"]) {
        MessageHandler::showMessages();
    }
    MessageHandler mh;
    
    // Suppress MessageHandler output in quiet mode
    if (!opt["quiet"]) {
        mh.begin("started");
    }

    Graph graph;
    std::vector<double> edge_prob_list;
    std::map<std::string, double> vertex_prob_map;
    try {
        if (!graphFileName.empty()) {
            if (opt["a"]) {
                graph.readAdjacencyList(graphFileName);
            }
            else {
                std::stringstream sstr;
                parse_graph_file(graphFileName, graph, edge_prob_list);
            }
        }

        if (!termFileName.empty() && !opt["allrel"]) {
            graph.readVertexGroups(termFileName);
        } else { // Make all vertices terminals
            for (int v = 1; v <= graph.edgeSize(); ++v) {
                graph.setColor(graph.vertexName(v), 1);
            }
            graph.update();
        }

        if (!edgeProbFileName.empty()) {
            std::ifstream ifs(edgeProbFileName.c_str());
            double v;
            int c = 0;
            while (ifs && ifs >> v && c < graph.edgeSize()) {
                edge_prob_list.push_back(v);
                ++c;
            }
            if (edge_prob_list.size() < static_cast<size_t>(graph.edgeSize())) {
                throw std::runtime_error("ERROR: please put probabilities!");
            }
        } else { // All probabilities are 0.5.
            if (edge_prob_list.empty()) {
                for (int i = 0; i < graph.edgeSize(); ++i) {
                    edge_prob_list.push_back(0.5);
                }
            }
        }

        // Load vertex probabilities if specified
        if (optStr.count("vertexfile") && !optStr["vertexfile"].empty()) {
            parse_vertex_prob_file(optStr["vertexfile"], vertex_prob_map);
        } else {
            // Default: all vertices have probability 0.5
            for (int v = 1; v <= graph.vertexSize(); ++v) {
                vertex_prob_map[graph.vertexName(v)] = 0.5;
            }
        }

        if (!opt["quiet"]) {
            mh << "#vertex = " << graph.vertexSize() << ", #edge = "
                << graph.edgeSize() << ", #color = "
                << graph.numColor() << "\n";
        }

        if (graph.edgeSize() == 0)
            throw std::runtime_error("ERROR: The graph is empty!");

        if (opt["graph"]) {
            graph.dump(std::cout);
            return 0;
        }

        // input confirmation here
#ifdef INPUT_CONFIRM_MODE
        std::cout << "=== INPUT CONFIRMATION MODE ===\n";
        std::cout << "Graph file: " << graphFileName << "\n";
        std::cout << "Terminal file: " << termFileName << "\n";
        std::cout << "Edge probability file: " << edgeProbFileName << "\n";
        
        std::cout << "\nGraph information:\n";
        std::cout << "  #vertex = " << graph.vertexSize() << "\n";
        std::cout << "  #edge = " << graph.edgeSize() << "\n";
        std::cout << "  #color = " << graph.numColor() << "\n";

        std::cout << "\nEdge list:\n";
        for (int i = 0; i < graph.edgeSize(); ++i) {
            Graph::EdgeInfo edge = graph.edgeInfo(i);
            std::cout << "  Edge " << i << ": " << graph.vertexName(edge.v1) << " (no " << edge.v1 << ")" << " -- " << graph.vertexName(edge.v2) << " (no " << edge.v2 << ")";
            if (edge.finalEdge) {
                std::cout << " (final)";
            }
            std::cout << "\n";
        }
        
        std::cout << "\nEdge probabilities (" << edge_prob_list.size() << " entries):\n";
        for (size_t i = 0; i < edge_prob_list.size(); ++i) {
            std::cout << "  Edge " << i << ": " << edge_prob_list[i] << "\n";
        }
        
        if (!vertex_prob_map.empty()) {
            std::cout << "\nVertex probabilities (" << vertex_prob_map.size() << " entries):\n";
            for (std::map<std::string, double>::const_iterator it = vertex_prob_map.begin();
                 it != vertex_prob_map.end(); ++it) {
                std::cout << "  Vertex " << it->first << ": " << it->second << "\n";
            }
        }
        
        if (optStr.count("vertexfile")) {
            std::cout << "\nVertex file option: " << optStr["vertexfile"] << "\n";
        }
        
        std::cout << "\nOptions set:\n";
        for (std::map<std::string, bool>::const_iterator it = opt.begin(); it != opt.end(); ++it) {
            if (it->second) {
                std::cout << "  -" << it->first << "\n";
            }
        }
        
#endif
        if (!opt["quiet"]) {
            mh << "---------- Edge reliability BDD construction start\n";
        }

        // look ahead cannot be used for BDDs,
        // so set the 4th argment to false
        FrontierBasedSearch fbs(graph, -1, false, false);
        DdStructure<2> dd;

        dd = DdStructure<2>(fbs);

        if (!opt["quiet"]) {
            mh << "---------- Edge reliability BDD construction end\n";
        }

        std::vector<double> edge_prob_rev_list(edge_prob_list.rbegin(), edge_prob_list.rend());
        edge_prob_rev_list.insert(edge_prob_rev_list.begin(), 0.0); // Add a dummy probability for the root node
        
        if (!opt["quiet"]) {
            mh << "\n#node = " << dd.size() << ", #solution = "
                    << std::setprecision(10)
                    << dd.evaluate(BddCardinality<double>(graph.edgeSize()))
                    << ", prob = "
                    << dd.evaluate(ProbEval(edge_prob_rev_list))
                    << "\n";
        }

        if (opt["reduce"]) {
            dd.bddReduce();
            if (!opt["quiet"]) {
                mh << "\n#node = " << dd.size() << ", #solution = "
                   << std::setprecision(10)
                   << dd.evaluate(BddCardinality<double>(graph.edgeSize()))
                   << ", prob = "
                   << dd.evaluate(ProbEval(edge_prob_rev_list))
                   << "\n";
            }
        }

        if (opt["count"]) {
            MessageHandler mh;
            if (!opt["quiet"]) {
                mh.begin("counting solutions") << " ...";
                mh << "\n#solution = " << dd.evaluate(BddCardinality<>(graph.edgeSize()));
                mh.end();
            }
        }

        if (opt["vertex"]) {
            if (opt["allrel"]) {
                if (!opt["quiet"]) {
                    mh << "ERROR: -allrel option is not compatible with -vertex option.\n";
                }
                return 1;
            }
            GlobalVariables gv;
            gv.graph = &graph;
            gv.edge_dd = &dd;
            gv.edge_prob_list = edge_prob_list;
            gv.vertex_prob_map = vertex_prob_map;
            
            if (!opt["quiet"]) {
                mh << "---------- Vertex reliability BDD construction start\n";
            }
            auto start_time = std::chrono::high_resolution_clock::now();
            // Run Kawahara et al.'s algorithm
            bddp vertex_dd = computeVertexReliability(gv);
            auto end_time = std::chrono::high_resolution_clock::now();
            if (!opt["quiet"]) {
                mh << "---------- Vertex reliability BDD construction end\n";
            }
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            double execution_time = duration.count() / 1000.0;

            BDD vertex_dd_s = BDD_ID(bddcopy(vertex_dd));
            tdzdd::SapporoBdd sapporo_bdd(vertex_dd_s);
            tdzdd::DdStructure<2> vertex_dd_structure = tdzdd::DdStructure<2>(sapporo_bdd);

            if (!opt["quiet"]) {
                mh << "Vertex reliability BDD construction time = " << execution_time << "\n";
                mh << "\n#node = " << bddsize(vertex_dd)
                   << ", prob = " << vertex_dd_structure.evaluate(ProbEval(gv.edge_vertex_prob_list))
                   << "\n";
            }

            if (opt["alg_k"]) {
                if (!opt["quiet"]) {
                    mh << "---------- alg_k start\n";
                }
                auto alg_k_start = std::chrono::high_resolution_clock::now();
                // Run Kuo et al.'s algorithm
                bddp h = alg_k(gv.shifted_edge_dd.GetID(), graph, graph.edgeSize(), graph.vertexSize(), gv.e_list, gv.v_list);
                auto alg_k_end = std::chrono::high_resolution_clock::now();
                if (!opt["quiet"]) {
                    mh << "---------- alg_k end\n";
                    double alg_k_time = std::chrono::duration_cast<std::chrono::milliseconds>(alg_k_end - alg_k_start).count() / 1000.0;
                    mh << "alg_k execution time = " << alg_k_time << " seconds\n";
                }
                
                if (opt["quiet"]) {
                    // In quiet mode, only output OK/NG
                    if (vertex_dd == h) {
                        std::cout << "OK" << std::endl;
                    } else {
                        std::cout << "NG" << std::endl;
                    }
                } else {
                    if (vertex_dd == h) {
                        mh << "alg_k result matches vertex reliability BDD.\n";
                    } else {
                        mh << "alg_k result does not match vertex reliability BDD.\n";
                    }
                }
            }
        }

        if (opt["zdd"]) dd.dumpDot(std::cout, "ZDD");
        if (opt["export"]) dd.dumpSapporo(std::cout);

        if (opt["solutions"]) {
            int count = optNum["solutions"];

            for (DdStructure<2>::const_iterator t = dd.begin(); t != dd.end();
                    ++t) {
                EdgeDecorator edges(graph.edgeSize(), *t);
                graph.dump(std::cout, edges);
                if (--count == 0) break;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    mh.end("finished");
    return 0;
}
