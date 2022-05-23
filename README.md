# Exercise 4

Fourth exercise proposed in the course "Scalable Computing" at FGV - EMAp. Using MPI in C++ to solve word counting problem.

Group members:
- Caio Lins
- João Pedro Donasolo
- João Vinicius Primaki
- Tomás Ferranti

First you need to compile main.cpp with 

    mpicxx main.cpp -o OUTPUT_FILE
    
command, after that just run the output with 

    mpiexec -n NUMBER_OF_PROCESSES OUTPUT_FILE
    
, where NUMBER_OF_PROCESSES is a number bigger than $2$.
