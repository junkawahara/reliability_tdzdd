Reliability
=================

### Computing network reliability with TdZdd library

This program computes network reliability for a given network.
The source code is a subset of [graphillion_example in TdZdd library](https://github.com/kunisura/TdZdd/tree/master/apps/graphillion) written by Hiroaki Iwashita.

## Usage

# Build

```
make
```

# Run

```
./reliability <graph_file> <terminal_file> <probability_file>
```

example

```
./reliability ./reliability.exe grid2x2.dat grid2x2_t.dat grid2x2_p.dat
```

## Link

* [TdZdd](https://github.com/kunisura/TdZdd/)
