#include <mpich/mpi.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

int main(int argc, char* argv[]) {
    
    clock_t tStart;
    int mpi_rank, mpi_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    

    srand(1); //for generating same values every time
    
    int sizePerProcess = 40000000;
    int sizeFull = mpi_size * sizePerProcess;

    int* arrFull; //full array
    int* arrPart = new int[sizePerProcess]; //part of array per process
    long* sums; //array of sums of partial arrays for process 0
    long sumPart=0; //sum of partial array of this process

    //main process
    if (mpi_rank == 0) {
        arrFull = new int[sizeFull];
        sums = new long[mpi_size];

        //generating full array
        for (int i = 0; i < sizeFull; i++) {
            arrFull[i] = rand();
        }
        

        std::cout << "=================";
        std::cout << "\nArray size = " << sizeFull;
        std::cout << "\nProcesses count = " << mpi_size;
        std::cout << "\nArray size per process = " << sizePerProcess;
        std::cout << "\n=================";
        
        //calc execution time
        tStart=clock();

    }

    //send parts of array
    MPI_Scatter(arrFull, sizePerProcess, MPI_INT, arrPart, sizePerProcess, MPI_INT, 0, MPI_COMM_WORLD);
    
    //sum of partial array
    for (int i = 0; i < sizePerProcess; i++) {
        sumPart+=arrPart[i];
    }
    
    //gather partial array sums
    MPI_Gather(&sumPart, 1, MPI_LONG, sums, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    //main process
    if (mpi_rank==0) {
        //sum partial sums
        long sumFull=0;
        for (int i = 0; i < mpi_size; i++) {
            sumFull+=sums[i];
        }
        
        std::cout << "\nParallel:";
        std::cout << "\nSum = " << sumFull;
        
        printf("\nTime taken: %.4fs", (double) (clock() - tStart) / CLOCKS_PER_SEC);
        tStart=clock();
        
        //calc sum with 1 process for test
        long test = 0;
        for (int i = 0; i < sizeFull; i++) {
            test+=arrFull[i];
        }
        std::cout << "\n=================";
        std::cout << "\nLinear:";
        std::cout << "\nSum = " << test;
        
        printf("\nTime taken: %.4fs", (double) (clock() - tStart) / CLOCKS_PER_SEC);
        std::cout << "\n=================";

    }
    
    MPI_Finalize();
    
    return 0;
}
