#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <thread>
#include <vector>
#include <mpi.h>

void manager(int my_rank, int world_size)
{
  std::cout << "Hi, I am the manager. My Rank is: " << my_rank << "/" << world_size-1 << std::endl;

  // Manager loads text data
  // * Open the input file and get the fileSize data
  std::ifstream file("shakespeare.txt", std::ios::ate);
  std::streamsize fileSize = file.tellg();
  if (fileSize == -1) 
  {
        perror("Failed to open the file. Maybe you forgot to have shakespeare.txt in your current folder?\n");
        exit(EXIT_FAILURE);
  }
  file.seekg(0, std::ios::beg);
  printf("File opened with %td bytes\n", fileSize);

  // * Load data from file
  std::vector<char> buffer(fileSize);
  if (!file.read(buffer.data(), fileSize)) 
  {
      perror("[ERROR] Failed to read the input file\n");
      exit(EXIT_FAILURE);
  }
  printf("Data loaded into buffer\n");

  // Manager sends the necessary size of text to each worker
  std::vector<int> nBytesData;
  std::vector<std::string> dataBlocks;
  
  // * Size the input
  int nWorkers = world_size-1;
  dataBlocks.resize(nWorkers);
  nBytesData.resize(nWorkers);

  // * Break data into blocks
  int blockSize = int(fileSize) / nWorkers;
  int blockStart = 0;
  int blockEnd = blockSize;
  for (int blockInx=0; blockInx < nWorkers; blockInx++) {

      // ** Look for the first character space in order to avoid splitting a word
      while (blockEnd < fileSize && buffer[blockEnd] != ' ') {
          blockEnd++;
      }

      // ** Create the block
      dataBlocks[blockInx] = std::string(&buffer[blockStart], blockEnd - blockStart);
      nBytesData[blockInx] = blockEnd-blockStart;
      printf("Created block #%d from %d to %d with %d bytes\n", blockInx, blockStart, blockEnd, nBytesData[blockInx]);
      MPI_Send( &nBytesData[blockInx], 1, MPI_INT, blockInx + 1, 69, MPI_COMM_WORLD );

      // ** Update indexes
      blockStart = blockEnd + 1;
      blockEnd = blockStart + blockSize;
      if (blockStart > fileSize) {
          break;
      } else if (blockEnd > fileSize) {
          blockEnd = fileSize;
      }
  }
  printf("All blocks were created\n");

  // Manager sends text

  // Manager keeps waiting to collect each worker total number of ocurrences
  std::vector<int> count(world_size-1, 0);
  

  // Manager sums and prints total number of ocurrences 
  
  std::cout << "Manager: " << my_rank << ". Message: DONE" << std::endl;
}

void worker(int my_rank, int world_size)
{
  std::cout << "Hi, I am a worker. My Rank is: " << my_rank << "/" << world_size-1 << std::endl;
  
  // Workers wait for specific file size message from manager
  int nBytes;
  MPI_Status status;
  MPI_Recv( &nBytes, 1, MPI_INT, 0, 69, MPI_COMM_WORLD, &status );
  std::cout << "Worker: " << my_rank << ". Message: allocated " << nBytes << " bytes" << std::endl;

  // Workers allocate memory to receive their part of text to process
  std::vector<char> my_text[nBytes];

  // Workers process and send final number of ocurrences to manager
  int my_ocurrences;
  std::cout << "Worker: " << my_rank << ". Message: DONE" << std::endl;

}

std::string FILENAME = "shakespeare.txt";

int main(int argc, char **argv) {
  // Initialize MPI
  // This must always be called before any other MPI functions
  // Abort if any problems arise from init
  if (MPI_Init(&argc,&argv) != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, 1);

  // Get the number of processes in MPI_COMM_WORLD
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get the rank of this process in MPI_COMM_WORLD
  int my_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  // Each rank has its task
  
  // CONTEXT: Manager is Rank 0, Workers are other Ranks
  // Manager loads text data
  // Workers wait for specific file size message from manager
  // Manager sends the necessary size of text to each worker
  // Workers allocate memory to receive their part of text to process
  // Manager sends text
  // Manager keeps waiting to collect each worker total number of ocurrences
  // Workers process and send final number of ocurrences
  // Manager sums and prints total number of ocurrences 
  
  if (my_rank == 0)
  {
    manager(my_rank, world_size);
  }else{
    worker(my_rank, world_size);
  } 

  // Finalize MPI
  // This must always be called after all other MPI functions
  std::cout << "Process: " << my_rank << ". Message: DONE" << std::endl;
  MPI_Finalize();

  return 0;
}