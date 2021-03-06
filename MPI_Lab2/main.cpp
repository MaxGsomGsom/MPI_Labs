#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <sstream> 

//tags for send/receive

enum Tags{
	createArrayTag = 0,
	shiftSizeTag = 1,
	shiftTag = 2,
	arraySizeTag = 3
};

//func shifts elems

void ShiftArrayFunc(int sizePerProcess, int shiftSize, int* array, int* newElems) {
	for (int i = sizePerProcess - 1; i >= shiftSize; i--) {
		array[i] = array[i - shiftSize];
	}

	for (int i = 0; i < shiftSize; i++) {
		array[i] = newElems[i];
	}
}

void MainProcessFunc(int mpi_rank, int mpi_size) {
	int sizePerProcess = 5;
	int shiftSize = 3;

	printf("Processes count = %d\n", mpi_size);
	printf("Array size per process = %d \nShift size = %d\n", sizePerProcess, shiftSize);

	//create array
	int* array = new int[mpi_size * sizePerProcess];

	printf("Full array: ");
	for (int i = 0; i < mpi_size * sizePerProcess; i++) {
		array[i] = rand() % 100;
		printf("%d-", array[i]);
	}


	//send array size
	for (int i = 1; i < mpi_size; i++) {
		MPI_Send(&sizePerProcess, 1, MPI_INT, i, arraySizeTag, MPI_COMM_WORLD);
	}

	//send array
	for (int i = 1; i < mpi_size; i++) {
		MPI_Send(&(array[(i - 1) * sizePerProcess]), sizePerProcess, MPI_INT, i, createArrayTag, MPI_COMM_WORLD);
	}

	//send shift array size
	for (int i = 1; i < mpi_size; i++) {
		MPI_Send(&shiftSize, 1, MPI_INT, i, shiftSizeTag, MPI_COMM_WORLD);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
}

void SecondaryProcessesFunc(int mpi_rank, int mpi_size) {
	//receive array size
	int sizePerProcess;
	MPI_Recv(&sizePerProcess, 1, MPI_INT, 0, arraySizeTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	//receive part of array
	int* array = new int[sizePerProcess];
	MPI_Recv(array, sizePerProcess, MPI_INT, 0, createArrayTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	//receive shift size
	int shiftSize;
	MPI_Recv(&shiftSize, 1, MPI_INT, 0, shiftSizeTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	
	//last process sends shift elems 
	if (mpi_rank == (mpi_size - 1))
	MPI_Send(&(array[sizePerProcess - shiftSize]), shiftSize, MPI_INT, 1, shiftTag, MPI_COMM_WORLD);
	//all processes except last send shift elems to next process
	else 
	MPI_Send(&(array[sizePerProcess - shiftSize]), shiftSize, MPI_INT, mpi_rank + 1, shiftTag, MPI_COMM_WORLD);

	//processes receive shift elems from previous process
	int* newElements = new int[shiftSize];
	if (mpi_rank == 1)
	MPI_Recv(newElements, sizePerProcess, MPI_INT, mpi_size - 1, shiftTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	else
	MPI_Recv(newElements, sizePerProcess, MPI_INT, mpi_rank - 1, shiftTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	
	
	std::stringstream ss;
	ss << "\n==========================================================";
	ss << "\nProcess rank = " << mpi_rank;
	ss << "\nBefore shift: ";
	for (int i = 0; i < sizePerProcess; i++) {
		ss << array[i] << "-";
	}

	//shift elems
	ShiftArrayFunc(sizePerProcess, shiftSize, array, newElements);
	

	ss << "\nAfter shift: ";
	for (int i = 0; i < sizePerProcess; i++) {
		ss << array[i] << "-";
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	usleep(mpi_rank*100);
	std::cout << ss.str();
}

int main(int argc, char* argv[]) {

	clock_t tStart = clock();

	int mpi_rank, mpi_size;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

	if (mpi_size < 3)
	return 0;
	
	srand(time(NULL)+mpi_rank);

	//main process
	if (mpi_rank == 0) {
		MainProcessFunc(mpi_rank, mpi_size);
	}
	//other processes
	else {
		SecondaryProcessesFunc(mpi_rank, mpi_size);
	}

	MPI_Finalize();
	
	
	if (mpi_rank==0) {
		printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
	}


	return 0;
}
