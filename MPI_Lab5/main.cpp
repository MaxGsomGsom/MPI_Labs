#include <mpich/mpi.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <stdint.h>

typedef uint8_t byte;

enum TAGS {
    tag_push = 10,
    tag_pop = 11
};


class FIFO {
public:
    FIFO(int max_data_size); //constructor
    bool Push(void* data, int size); //put element to puffer
    int Pop(byte* result); //get element form buffer
    int Count(); //count of elements
private:
    byte* data; //data of element
    int mpi_rank, mpi_size; 
    int cur_count; //cur elements count in buffer
    int max_data_size; //max size of element
};

FIFO::FIFO(int max_data_size) {
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    cur_count = 0;
    data = new byte[max_data_size + 4];
    this->max_data_size = max_data_size;
}

bool FIFO::Push(void* input, int size) {
    //if there is no more space in buffer
    if (cur_count == mpi_size) {
        return false;
    }
    if (mpi_rank == 0) {
        //copy size and data of element to temp memory
        byte* buf = new byte[max_data_size + 4];
        memcpy(buf, &size, 4);
        memcpy(&(buf[4]), input, size);

        //send new element to process with rank equal to current count of elements
        if (cur_count > 0) {
            MPI_Send(buf, max_data_size + 4, MPI_BYTE, cur_count, tag_push, MPI_COMM_WORLD);
        //if its first element, just copy memory of process 0
        } else {
            memcpy(data, buf, max_data_size + 4);
        }
        delete buf;
    //put element to process with rank equal to current count of elements
    } else if (mpi_rank == cur_count) {
        MPI_Recv(data, max_data_size + 4, MPI_BYTE, 0, tag_push, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    cur_count++;
    MPI_Barrier(MPI_COMM_WORLD);
    return true;
}

int FIFO::Pop(byte *result) {
    //if there is no elements in buffer
    if (cur_count == 0)
        return -1;

    if (mpi_rank == 0) {
        //copy size and element data in process 0 memory to output
        int size;
        memcpy(&size, data, 4);
        memcpy(result, &(data[4]), size);

        //if there is more then 1 element, need shift all others elements to previous processes
        //so wait for element from next process
        if (cur_count > 1)
            MPI_Recv(data, max_data_size, MPI_BYTE, 1, tag_pop, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        cur_count--;
        MPI_Barrier(MPI_COMM_WORLD);
        return size;
    //last process with element sends it to previous process
    } else if (mpi_rank == (cur_count - 1)) {
        MPI_Send(data, max_data_size, MPI_BYTE, mpi_rank - 1, tag_pop, MPI_COMM_WORLD);
    //all the others processes with elements send them to previous processes
    //and receive elements from next process
    } else if (mpi_rank < (cur_count - 1)) {
        MPI_Send(data, max_data_size, MPI_BYTE, mpi_rank - 1, tag_pop, MPI_COMM_WORLD);
        MPI_Recv(data, max_data_size, MPI_BYTE, mpi_rank + 1, tag_pop, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    cur_count--;
    MPI_Barrier(MPI_COMM_WORLD);
    return 0;
}

int FIFO::Count() {
    return cur_count;
}

int main(int argc, char* argv[]) {
    int mpi_rank, mpi_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    int data_size = 100;
    int pop_push_count = mpi_size + 2-8; //how many elements generate for test

    FIFO* buffer = new FIFO(data_size);

    if (mpi_rank == 0) {
        std::cout << "Max elements in buffer = " << mpi_size << "\n";
        std::cout << "Max data size = " << data_size << "\n";
    }

    //generate and push some elements
    for (int i = 0; i < pop_push_count; i++) {
        int n;
        char* input;

        //generate data
        if (mpi_rank == 0) {
            n = rand() % 3 + 2;
            input = new char[n + 1];

            for (int i = 0; i < n; i++) {
                input[i] = rand() % 25 + 65;
            }
            input[n] = '\0';
        }

        //push to buffer
        bool flag = buffer->Push(&input, n);

        //print result
        if (mpi_rank == 0) {
            if (flag)
                std::cout << "Push data: " << input << ". Elements in buffer = " << buffer->Count() << "\n";
            else
                std::cout << "Push data. " << input << ". Cant push. Elements in buffer = " << buffer->Count() << "\n";
        }
    }

    //pop some elements
    for (int i = 0; i < pop_push_count; i++) {

        int result_size;
        byte *result = new byte[data_size];
        
        //pop from puffer
        result_size = buffer->Pop(result);

        //print result
        if (mpi_rank == 0) {
            if (result_size == -1)
                std::cout << "Pop data. Cant pop. Elements in buffer = " << buffer->Count() << "\n";
            else {
                char* output;
                memcpy(&output, result, result_size);
                std::cout << "Pop data: " << output << ". Size = " << result_size << ". Elements in buffer = " << buffer->Count() << "\n";
            }
        }
    }

    MPI_Finalize();
    return 0;
}
