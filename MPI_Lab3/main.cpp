#include <mpich/mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <string.h>


typedef uint8_t byte;

//tags for messages
enum MPI_Tags {
    messageTag = 0,
    confirmTag = 1
};

void MainProcessFunc(int mpi_rank, int mpi_size) {

    MPI_Status status;
    int count;
    byte* message = new byte;
    int destination;

    printf("Processes count = %d\n", mpi_size);

    while (true) {
        //wait for message
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_BYTE, &count);

        delete message;
        message = new byte[count];

        //resend message
        MPI_Recv(message, count, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        memcpy(&destination, &(message[4]), 4);
        
        MPI_Send(message, count, MPI_BYTE, destination, status.MPI_TAG, MPI_COMM_WORLD);

    }

}

void SecondaryProcessesFunc(int mpi_rank, int mpi_size) {

    MPI_Status status;
    int flag;
    time_t prevTime;
    int dTime;
    int count;
    byte* message = new byte;
    int source, destination;
    byte* responseMessage = new byte;
    int messageID = mpi_rank*100;
    int receivMessID;

    while (true) {
        
        prevTime = time(NULL);
        dTime = random() % mpi_size * 2 + 2;
        
        //wait some time for message
        while (time(NULL)<(prevTime + dTime)) {
            MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

            //if there is message
            if (flag) {
                MPI_Get_count(&status, MPI_BYTE, &count);

                delete message;
                message = new byte[count];

                //receive message
                MPI_Recv(message, count, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                memcpy(&source, &(message[0]), 4);
                memcpy(&receivMessID, &(message[8]), 4);

                //if message with data - send confirmation
                if (status.MPI_TAG == messageTag) {
                    printf("[%d] received message from [%d]. ID: %d. Data: %s\n", mpi_rank, source, receivMessID, &(message[12]));
                    
                    delete responseMessage;
                    responseMessage = new byte[12];
                    
                    memcpy(responseMessage, &mpi_rank, 4);
                    memcpy(&(responseMessage[4]), &source, 4);
                    memcpy(&(responseMessage[8]), &receivMessID, 4);
                    
                    MPI_Send(responseMessage, 12, MPI_BYTE, 0, confirmTag, MPI_COMM_WORLD);
                //if confirmation - print to screen
                } else if (status.MPI_TAG == confirmTag) {
                    printf("[%d] received confirmation from [%d]. ID: %d\n", mpi_rank, source, receivMessID);
                }

                flag = 0;
            }

        }
  
        //create new random message after waiting for some time
        messageID++;
        delete message;
        //random length
        count = random() % 10 + 14;
        message = new byte[count];
        
        //random data
        for (int i = 12; i < count - 1; i++) {
            message[i] = random() % 25 + 65;
        }
        message[count - 1] = '\0';

        //random destination
        destination = mpi_rank;
        for (; destination == mpi_rank;) {
            destination = random() % (mpi_size - 1) + 1;
        }

        //send message
        memcpy(message, &mpi_rank, 4);
        memcpy(&(message[4]), &destination, 4);
        memcpy(&(message[8]), &messageID, 4);

        printf("[%d] sent message to [%d]. ID: %d. Data: %s\n", mpi_rank, destination, messageID, &(message[12]));

        MPI_Send(message, count, MPI_BYTE, 0, messageTag, MPI_COMM_WORLD);

    }

}


int main(int argc, char* argv[]) {
    int mpi_rank, mpi_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    //if not enough processes
    if (mpi_size < 3)
        return 0;

    srand(time(NULL)+mpi_rank);
    
    //main process
    if (mpi_rank == 0) {
        MainProcessFunc(mpi_rank, mpi_size);
    }//other processes
    else {
        SecondaryProcessesFunc(mpi_rank, mpi_size);
    }

    MPI_Finalize();
    return 0;
}
