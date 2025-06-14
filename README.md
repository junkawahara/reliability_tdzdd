Reliability
=================

# Program that computes network reliability using TdZdd library

This program computes network reliability for a given network.
The source code provided here is a subset of [graphillion_example in TdZdd library](https://github.com/kunisura/TdZdd/tree/master/apps/graphillion) written by Dr. Hiroaki Iwashita.

## Usage

### Build

```
make
```

### Run

```
./reliability [options] <graph_file> <terminal_file> <probability_file>
```

### Options

* `-vertex` : Compute the reliability with imperfect vertices (both vertices and edges can fail)
* `-alg_k` : Run Kuo et al.'s algorithm (used with `-vertex` option to compare results)
* `--vertexfile=<filename>` : Specify vertex failure probability file when using `-vertex` option
* `-a` : Read <graph_file> as an adjacency list
* `-allrel` : Compute all terminal reliability (ignoring <terminal_file>)
* `-count` : Report the number of solutions
* `-graph` : Dump input graph to STDOUT in DOT format
* `-reduce` : Reduce result BDD
* `-solutions <n>` : Dump at most <n> solutions to STDOUT in DOT format
* `-zdd` : Dump result ZDD to STDOUT in DOT format
* `-export` : Dump result ZDD to STDOUT

### Examples

Basic usage:
```
./reliability grid2x2.dat grid2x2_t.dat grid2x2_p.dat
```

With vertex reliability:
```
./reliability -vertex --vertexfile=vertex_prob.dat grid2x2.dat grid2x2_t.dat
```

With vertex reliability and alg_k comparison:
```
./reliability -vertex -alg_k --vertexfile=vertex_prob.dat grid2x2.dat grid2x2_t.dat
```

Using graph file with embedded edge probabilities:
```
./reliability grid2x2_with_prob.dat grid2x2_t.dat
```

## File format

### graph_file

The graph file supports two formats:

#### Format 1: Edge list only (traditional format)
```
1 2
1 3
2 3
2 4
```

#### Format 2: Edge list with probabilities (enhanced format)
```
1 2 0.9
1 3 0.8
2 3 0.7
2 4 0.6
```

The format of <graph_file> is a so-called edge list. One line corresponds to an edge
and consists of two or three values:
- Two integers representing the vertex numbers of the endpoints
- Optional third value representing the edge failure probability

In the above examples, there are four vertices 1, 2, 3 and 4, and four edges
connecting 1 with 2, 1 with 3, 2 with 3 and 2 with 4.

When using the 3-column format, the third column specifies the probability that
the edge is available (not failed). If only 2 columns are provided, edge
probabilities can be specified using the probability_file parameter.

### terminal_file

Example:

```
1 4
```

The terminal_file specifies vertex numbers all of which must be connected.
Each number is separated by a white space.

If neither terminal_file nor probability_file arguments are specified,
the terminal is set to be all the vertices, that is, all terminal reliability
are computed.

### probability_file

```
0.9 0.8 0.7 0.6
```

The probability_file specifies the probabilities of the availabilities of edges.
The i-th real value indicates the probability that the i-th edge is available (not failure).
Each value is separated by a white space.

If the probability_file argument is not specified, all the probabilities
are set to be 0.5.

Note: This file is not needed when using the 3-column graph_file format.

### vertex_file (for vertex reliability)

Example:

```
1 0.95
2 0.90
3 0.85
4 0.88
```

The vertex_file specifies the probabilities of vertex availabilities when using
the `-vertex` option. Each line contains:
- Vertex name/number
- Probability that the vertex is available (not failed)

Values can be separated by spaces or commas. This file is specified using the
`--vertexfile=<filename>` option.

## Link

* [TdZdd](https://github.com/kunisura/TdZdd/)
* [SAPPOROBDD](https://github.com/Shin-ichi-Minato/SAPPOROBDD)
* K. Sekine, H. Imai, and S. Tani, ``Computing the Tutte polynomial of a graph
  of moderate size,'' in Proceedings of the 6th International Symposium
  on Algorithms and Computation (ISAAC), 1995, pp. 224--233.
* G. Hardy, C. Lucet, and N. Limnios, ``K-terminal network reliability measures
  with binary decision diagrams,'' IEEE Transactions on Reliability,
  vol. 56, no. 3, pp. 506--515, September 2007.
* S.-Y. Kuo, F.-M. Yeh and H.-Y. Lin, ``Efficient and Exact Reliability Evaluation for Networks With Imperfect Vertices,'' IEEE Transactions on Reliability, vol. 56, no. 2, pp. 288--300, June 2007, doi: 10.1109/TR.2007.896770.
