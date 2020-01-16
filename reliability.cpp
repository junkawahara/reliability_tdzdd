/*
 * Reliability: Computing the network reliability with TdZdd
 * by Jun Kawahara
 * Copyright (c) 2017 Jun Kawahara
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

#include <climits>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fstream>

#include <tdzdd/DdSpecOp.hpp>
#include <tdzdd/DdStructure.hpp>

#include <tdzdd/util/Graph.hpp>
#include <tdzdd/spec/FrontierBasedSearch.hpp>

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
        {"export", "Dump result ZDD to STDOUT"}}; //

std::map<std::string,bool> opt;
std::map<std::string,int> optNum;
std::map<std::string,std::string> optStr;

class ProbEval: public tdzdd::DdEval<ProbEval,double> {
private:
    std::vector<double> prob_list_;
    
public:
    ProbEval(const std::vector<double>& prob_list) : prob_list_(prob_list) { }
    
    void evalTerminal(double& p, bool one) const {
        p = one ? 1.0 : 0.0;
    }

    void evalNode(double& p, int level, DdValues<double, 2> const& values) const {
        double pc = prob_list_[prob_list_.size() - level];
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

int main(int argc, char *argv[]) {

    for (unsigned i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        opt[options[i][0]] = false;
    }

    std::string graphFileName;
    std::string termFileName;
    std::string probFileName;

    try {
        for (int i = 1; i < argc; ++i) {
            std::string s = argv[i];
            if (s[0] == '-') {
                s = s.substr(1);

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
            else if (probFileName.empty()) {
                probFileName = s;
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

    MessageHandler::showMessages();
    MessageHandler mh;
    mh.begin("started");

    Graph g;
    std::vector<double> prob_list;
    try {
        if (!graphFileName.empty()) {
            if (opt["a"]) {
                g.readAdjacencyList(graphFileName);
            }
            else {
                g.readEdges(graphFileName);
            }
        }

        int const m = g.vertexSize();
        int const n = g.edgeSize();

        if (!termFileName.empty() && !opt["allrel"]) {
            g.readVertexGroups(termFileName);
        } else { // Make all vertices terminals
            for (int v = 1; v <= m; ++v) {
                g.setColor(g.vertexName(v), 1);
            }
            g.update();
        }

        if (!probFileName.empty()) {
            std::ifstream ifs(probFileName.c_str());
            double v;
            int c = 0;
            while (ifs && ifs >> v && c < n) {
                prob_list.push_back(v);
                ++c;
            }
            if (prob_list.size() < g.edgeSize()) {
                throw std::runtime_error("ERROR: please put probabilities!");
            }
        } else { // All probabilities are 0.5.
            for (int i = 0; i < n; ++i) {
                prob_list.push_back(0.5);
            }
        }

        mh << "#vertex = " << m << ", #edge = " << n << ", #color = "
           << g.numColor() << "\n";

        if (g.edgeSize() == 0)
            throw std::runtime_error("ERROR: The graph is empty!");

        if (opt["graph"]) {
            g.dump(std::cout);
            return 0;
        }

        // look ahead cannot be used for BDDs,
        // so set the 4th argment to false
        FrontierBasedSearch fbs(g, -1, false, false);
        DdStructure<2> dd;

        dd = DdStructure<2>(fbs);

        mh << "\n#node = " << dd.size() << ", #solution = "
                << std::setprecision(10)
                << dd.evaluate(BddCardinality<double>(n))
                << ", prob = "
                << dd.evaluate(ProbEval(prob_list))
                << "\n";

        if (opt["reduce"]) {
            dd.bddReduce();
            mh << "\n#node = " << dd.size() << ", #solution = "
               << std::setprecision(10)
               << dd.evaluate(BddCardinality<double>(n))
               << ", prob = "
               << dd.evaluate(ProbEval(prob_list))
               << "\n";
        }

        if (opt["count"]) {
            MessageHandler mh;
            mh.begin("counting solutions") << " ...";
            mh << "\n#solution = " << dd.evaluate(BddCardinality<>(n));
            mh.end();
        }

        if (opt["zdd"]) dd.dumpDot(std::cout, "ZDD");
        if (opt["export"]) dd.dumpSapporo(std::cout);

        if (opt["solutions"]) {
            int const n = g.edgeSize();
            int count = optNum["solutions"];

            for (DdStructure<2>::const_iterator t = dd.begin(); t != dd.end();
                    ++t) {
                EdgeDecorator edges(n, *t);
                g.dump(std::cout, edges);
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
