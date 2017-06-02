# CIS 452 Program 2 - Vector Processor Subtraction
This program was meant to demostrate a working knowledge of using multiple processes and inter process communication using pipes. The goal was to emulate a vector processor by performing an operation between two streams of numbers, or vectors, and outputting a vector with all the results. In this case, the operation we wanted to perform was subtraction. 

In order to emulate subtraction performed by a CPU, we need to take the 2's complement of the number we just read from the vector stream (vector A), and add it to the number read from the other vector stream (vector B). Each process spawned by the program was one step in this pipeline:
* Process 1 was the Complementer - flips all the bits in a binary number.
* Process 2 was the Incrementer - adds 1 to the number sent from the Complementer; we now have the 2's complement.
* Process 3 was the Adder - adds the 2's complemented number to the corresponding number in vector B; we just performed subtraction.

Each entry read in from the vectors would propogate through each process in order, emulating a pipelined processor, like so:
```
Complementer   >   Incrementer   >   Adder
bits flipped   >   added to 1    >   result obtained
```

The Adder process would then write the resulting numbers out to a result vector, with each number being appended to the end of the vector.

### Usage
```
vector <number_size> <vector_size> <vector_A> <vector_B> <out_file>
```

The program must be informed of the number size (how many bits long each number is) and the vector size (how many binary numbers are in each vector).  Vector_A and B must both be text files containing a list of binary numbers separated by newlines and no other whitespace.  The number size and vector length must be the same between vector_A and vector_B, and the number size must be the same for every number in a vector.

### Issues
* Has trouble with numbers that are 64 bits in length and longer

Todo.....
