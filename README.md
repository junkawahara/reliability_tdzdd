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
./reliability <graph_file> <terminal_file> <probability_file>
```

example

```
./reliability ./reliability.exe grid2x2.dat grid2x2_t.dat grid2x2_p.dat
```

## File format

### graph_file

Example:

```
1 2
1 3
2 3
2 4
```

The format of <graph_file> is a so-called edge list. One line corresponds to an edge
and consists of two integers representing the vertex numbers of the endpoints.
In the above example, there are four vertices 1, 2, 3 and 4, and four edges
connecting 1 with 2, 1 with 3, 2 with 3 and 2 with 4.

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

If the probability_file argument is not specifed, all the probabilities
are set to be 0.5.

## Link

* [TdZdd](https://github.com/kunisura/TdZdd/)
* K. Sekine, H. Imai, and S. Tani, ``Computing the Tutte polynomial of a graph
  of moderate size,'' in Proceedings of the 6th International Symposium
  on Algorithms and Computation (ISAAC), 1995, pp. 224--233.
* G. Hardy, C. Lucet, and N. Limnios, ``K-terminal network reliability measures
  with binary decision diagrams,'' IEEE Transactions on Reliability,
  vol. 56, no. 3, pp. 506--515, September 2007.
