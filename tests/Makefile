all: message

message:
	echo "Run make with one of intraprocedural-linked-list-benchmark test interprocedural-benchmark"

intraprocedural-linked-list-benchmark: intraprocedural-linked-list-benchmark.c
	clang -S -emit-llvm -fno-discard-value-names -Xclang -disable-O0-optnone -O0 ./intraprocedural-linked-list-benchmark.c -o ./intraprocedural-linked-list-benchmark.ll
	opt -S -mem2reg ./intraprocedural-linked-list-benchmark.ll -o ./intraprocedural-linked-list-benchmark.ll
	opt -S -load ../build/libmemorygraph.so -load ../build/libheapliveness.so -heap-liveness ./intraprocedural-linked-list-benchmark.ll -o ./intraprocedural-linked-list-benchmark.ll
	clang ./intraprocedural-linked-list-benchmark.ll -o intraprocedural-linked-list-benchmark
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes ./intraprocedural-linked-list-benchmark

correctness-check-benchmark: correctness-check-benchmark.c
	clang -S -emit-llvm -fno-discard-value-names -Xclang -disable-O0-optnone -O0 ./correctness-check-benchmark.c -o ./correctness-check-benchmark.ll
	opt -S -mem2reg ./correctness-check-benchmark.ll -o ./correctness-check-benchmark.ll
	opt -S -load ../build/libmemorygraph.so -load ../build/libheapliveness.so -heap-liveness ./correctness-check-benchmark.ll -o ./correctness-check-benchmark.ll
	clang ./correctness-check-benchmark.ll -o correctness-check-benchmark
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes ./correctness-check-benchmark

interprocedural-benchmark: interprocedural-benchmark.c
	clang -S -emit-llvm -fno-discard-value-names -Xclang -disable-O0-optnone -O0 ./interprocedural-benchmark.c -o ./interprocedural-benchmark.ll
	opt -S -mem2reg ./interprocedural-benchmark.ll -o ./interprocedural-benchmark.ll
	opt -S -load ../build/libmemorygraph.so -load ../build/libheapliveness.so -heap-liveness ./interprocedural-benchmark.ll -o ./interprocedural-benchmark.ll
	clang ./interprocedural-benchmark.ll -o interprocedural-benchmark
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes ./interprocedural-benchmark


clean:
	rm *.ll intraprocedural-linked-list-benchmark correctness-check-benchmark interprocedural-benchmark