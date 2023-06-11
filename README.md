# FALSE NINE

False Nine is a compile-time instrumentation project to insert calls to free as soon as a heap object is dead. It is based on liveness-based heap analysis approached mentioned in the "Data Flow Analysis - Theory and Practice" book.

## How to build

### Requirements
- cmake (tested on version 3.16.3)
- llvm (tested on version 10.0.0)
- valgrind-3.15.0 (To check if the pass introduces any correctness issues in the program)
- clang (tested on version 10.0.0)
- clang++ (tested on version 10.0.0)


### To build the compiler passes.
```bash
mkdir build
cd build
cmake ..
make -j
```

### To run the tests
The tests generate the binary and the IR file for the benchmark. They also run each binary with valgrind to check for any memory errors generated due to frees inserted.

```bash
cd tests
make <correctness-check-benchmark/interprocedural-benchmark/intraprocedural-linked-list-benchmark>
```

The compiler pass prints the number of frees inserted in the program. We use the intraprocedural-linked-list-benchmark to plot for the accuracy experiment and vary the number of elemnts in the linked list manually and plot the graph.

- The pass might crash for a larger number of elements in the linked list. This is because of the large amount of access paths ganerated in the IR for each access.

