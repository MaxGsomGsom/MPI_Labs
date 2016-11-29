#include <mpich/mpi.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

enum Tags {
    tag0 = 0,
    tag1 = 1
};

//matrix B is transposed!

int main(int argc, char* argv[]) {

    int matrixRank = 600;
    int maxNumsInMatrix = 100;
    
    clock_t tStart;
    int mpi_rank, mpi_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);


    srand(1); //for generation same values every time

    int linesInTask = matrixRank / mpi_size;

    long sizeFull = matrixRank * matrixRank;
    int *matrixA, *matrixB, *matrixC;

    if (sizeFull != matrixRank * linesInTask * mpi_size) {
        if (mpi_rank == 0) std::cout << "\nArray can't be divided between processes";
        return 0;
    }

    //main process
    if (mpi_rank == 0) {

        std::cout << "\n=================";
        std::cout << "\nMatrix rank = " << matrixRank;
        std::cout << "\nProcesses count = " << mpi_size;
        std::cout << "\nMatrix lines per process = " << linesInTask;

        matrixA = new int[sizeFull];
        matrixB = new int[sizeFull];
        matrixC = new int[sizeFull];

        //generation of matrix A and B
        for (long i = 0; i < sizeFull; i++) {
            matrixA[i] = rand() % maxNumsInMatrix;
            matrixB[i] = rand() % maxNumsInMatrix;
        }


        std::cout << "\nMatrix A first elements: ";
        for (int i = 0; i < 10; i++) {
            std::cout << matrixA[i] << " ";
        }
        std::cout << "\nMatrix B first elements: ";
        for (int i = 0; i < 10; i++) {
            std::cout << matrixB[i] << " ";
        }

        std::cout << "\n=================";
        std::cout << "\nLinear:";

        //calculations time
        tStart = clock();


        for (int i = 0; i < matrixRank; i++) {
            for (int j = 0; j < matrixRank; j++) {
                matrixC[i * matrixRank + j] = 0;
                for (int k = 0; k < matrixRank; k++) {
                    matrixC[i * matrixRank + j] += matrixA[i * matrixRank + k] * matrixB[j * matrixRank + k];
                }
            }
        }

        printf("\nTime taken: %.4fs", (double) (clock() - tStart) / CLOCKS_PER_SEC);

        std::cout << "\nMatrix C first elements: ";
        for (int i = 0; i < 10; i++) {
            std::cout << matrixC[i] << " ";
        }

        std::cout << "\n=================";
        std::cout << "\nParallel ribbon method:";
        tStart = clock();


    }

    MPI_Barrier(MPI_COMM_WORLD);

    int elemsPerTask = matrixRank*linesInTask;

    int* bufferA = new int[elemsPerTask];
    int* bufferB = new int[elemsPerTask];
    int* bufferC = new int[elemsPerTask];


    MPI_Scatter(matrixA, elemsPerTask, MPI_INT, bufferA, elemsPerTask, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(matrixB, elemsPerTask, MPI_INT, bufferB, elemsPerTask, MPI_INT, 0, MPI_COMM_WORLD);

    int shift, col, row, el;

    for (int i = 0; i < mpi_size; i++) {

        for (col = 0; col < linesInTask; col++) {
            for (row = 0; row < linesInTask; row++) {

                shift = (i + mpi_rank) % mpi_size * linesInTask;
                bufferC[row * matrixRank + col + shift] = 0;

                for (el = 0; el < matrixRank; el++) {
                    bufferC[row * matrixRank + col + shift] += bufferA[row * matrixRank + el] * bufferB[col * matrixRank + el];
                }
            }
        }

        if (i < (mpi_size - 1)) {
            MPI_Sendrecv(bufferB, elemsPerTask, MPI_INT, (mpi_rank == 0) ? (mpi_size - 1) : (mpi_rank - 1), tag1,
                    bufferB, elemsPerTask, MPI_INT, (mpi_rank == (mpi_size - 1)) ? 0 : (mpi_rank + 1), tag1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(bufferC, elemsPerTask, MPI_INT, matrixC, elemsPerTask, MPI_INT, 0, MPI_COMM_WORLD);


    if (mpi_rank == 0) {
        printf("\nTime taken: %.4fs", (double) (clock() - tStart) / CLOCKS_PER_SEC);
        std::cout << "\nMatrix C first elements: ";
        for (int i = 0; i < 10; i++) {
            std::cout << matrixC[i] << " ";
        }
        std::cout << "\n=================\n";
    }


    delete bufferA, bufferB, bufferC;
    if (mpi_rank == 0) delete matrixA, matrixB, matrixC;

    MPI_Finalize();

    return 0;
}
