#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <cstring>
#include <mpich/mpi.h>

void manager(int my_rank, int world_size, std::string FILENAME, std::string SEARCH_WORD)
{
  std::cout << "Hi, I am the manager. My Rank is: " << my_rank << "/" << world_size-1 << std::endl;

  // Manager loads text data
  // * Open the input file and get the fileSize data
  std::ifstream file(FILENAME, std::ios::ate);
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
  std::vector<std::string> dataBlocks;
  
  // * Size the input
  int nWorkers = world_size-1;
  dataBlocks.resize(nWorkers);
  int nBytesData[nWorkers];


  // * Break data into blocks
  int blockSize = int(fileSize) / nWorkers;
  std::vector<int> blockStart(nWorkers+1, 0);
  int blockEnd = blockSize;
  for (int blockInx=0; blockInx < nWorkers; blockInx++) 
  {
    // ** Look for the first character space in order to avoid splitting a word
    while (blockEnd < fileSize && buffer[blockEnd] != ' ') 
    {
      blockEnd++;
    }

    // ** Create the block
    dataBlocks[blockInx] = std::string(&buffer[blockStart[blockInx]], blockEnd - blockStart[blockInx]);
    nBytesData[blockInx] = dataBlocks[blockInx].size();
    printf("Created block #%d from %d to %d with %d bytes\n", blockInx, blockStart[blockInx], blockEnd, nBytesData[blockInx]);

    // ** Send initial index and block size
    MPI_Send( &(blockStart[blockInx]), 1, MPI_INT, blockInx + 1, 68, MPI_COMM_WORLD );
    MPI_Send( &(nBytesData[blockInx]), 1, MPI_INT, blockInx + 1, 69, MPI_COMM_WORLD );

    // ** Update indexes
    blockStart[blockInx+1] = blockEnd + 1;
    blockEnd = blockStart[blockInx+1] + blockSize;
    if (blockStart[blockInx+1] > fileSize) 
    {
      break;
    }      
    if (blockEnd > fileSize) 
    {
      blockEnd = fileSize;
    }
  }
  printf("All blocks were created\n");

  
  std::cout << "Manager: " << my_rank << ". Message: Sending data blocks to workers." << std::endl;
  // Manager sends text
  for (int blockInx=0; blockInx < nWorkers; blockInx++) 
  {
    MPI_Send(dataBlocks[blockInx].c_str(), nBytesData[blockInx], MPI_CHAR, blockInx + 1, 24, MPI_COMM_WORLD );
  } 

  // Manager keeps waiting to collect each worker total number of ocurrences
  // Manager sums and prints total number of ocurrences 
  std::cout << "Manager: " << my_rank << ". Message: Waiting for workers to send counts (ascending order)." << std::endl;
  int totalOcur = 0;
  int count = 0;
  MPI_Status status[nWorkers];
  for (int blockInx=0; blockInx < nWorkers; blockInx++) 
  {
    MPI_Recv(&count, 1, MPI_INT, blockInx + 1, 42, MPI_COMM_WORLD, &status[blockInx] );
    totalOcur += count;
    count = 0;
  } 
  
  std::cout << "The word " << SEARCH_WORD << " ocurred " << totalOcur << " times." << std::endl;
  std::cout << "Manager: " << my_rank << ". Message: DONE" << std::endl;
}

// All letters of the alphabet
std::string ALPHABET_LETTERS = "abcdefghijklmnopqrstuvwxyz";

/** Function to be used in the finding of words
 * Returns true for non alphabet characters
 */
bool notAlphabetLetter(char nextLetter)
{
  // Next char should not be a letter
  for (char l : ALPHABET_LETTERS)
  {
    if (nextLetter == l)
    {
      return false;
    }
  }
  return true;
}

/** Function to be used to count word ocurrences on string of char pointer
 * Returns the total count of words in the text
 */
int countWordInChar(int initialInx, char * textChar, std::string SEARCH_WORD)
{
  int wordCount = 0;
  int wordLength = SEARCH_WORD.size();
  int endPos = strlen(textChar);
  std::string text(textChar, endPos);
  int currentPos = 0;
  std::cout << "wordLenght: " << wordLength << std::endl; 
  std::cout << "endPos: " << endPos << std::endl; 
  std::cout << "SEARCH_WORD[0]: " << SEARCH_WORD[0] << std::endl; 
  std::cout << "SEARCH_WORD[1]: " << SEARCH_WORD[1] << std::endl; 
  while (currentPos < endPos) 
  { 
    if (text[currentPos] == SEARCH_WORD[0]) 
    {
      int wordIndex = 0;
      while(currentPos < endPos && text[currentPos] == SEARCH_WORD[wordIndex]) 
      {
        wordIndex++;
        currentPos++;
        if (notAlphabetLetter(text[currentPos]) && notAlphabetLetter(text[currentPos-(wordLength+1)]) && wordIndex == wordLength) 
        { 
          std::cout << "Word FOUND at index " << initialInx + currentPos - wordLength << " of the file." << std::endl;
          wordCount++;
          break;
        }
      }
    } else {
      currentPos++;
    }
  }
  return wordCount;
}

void worker(int my_rank, int world_size, std::string SEARCH_WORD)
{
  std::cout << "Hi, I am a worker. My Rank is: " << my_rank << "/" << world_size-1 << std::endl;
  
  // Workers wait for specific file size message from manager
  int nBytes;
  int startInx;
  MPI_Status status1;
  MPI_Status status2;
  MPI_Recv( &nBytes, 1, MPI_INT, 0, 69, MPI_COMM_WORLD, &status1 );
  MPI_Recv( &startInx, 1, MPI_INT, 0, 68, MPI_COMM_WORLD, &status2 );
  std::cout << "Worker: " << my_rank << ". Message: Start at index " << startInx << " bytes." << std::endl;
  std::cout << "Worker: " << my_rank << ". Message: Size of " << nBytes << " bytes." << std::endl;

  // Workers allocate memory to receive their part of text to process
  char* my_text;
  my_text = (char *) malloc(nBytes+1);
  MPI_Status status3;
  MPI_Recv( my_text, nBytes, MPI_CHAR, 0, 24, MPI_COMM_WORLD, &status3 );
  std::cout << "Worker: " << my_rank << ". Message: received data." << std::endl;

  // Workers process and send final number of ocurrences to manager
  int my_ocurrences;
  std::cout << "Worker: " << my_rank << ". Message: processing data." << std::endl;
  my_ocurrences = countWordInChar(startInx, my_text, SEARCH_WORD);
  MPI_Send(&my_ocurrences, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
  
  std::cout << "Worker: " << my_rank << ". Message: DONE" << std::endl;
}

int main(int argc, char **argv) {
  // Initialize MPI
  // This must always be called before any other MPI functions
  // Abort if any problems arise from init
  if (MPI_Init(&argc,&argv) != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, 1);

  
  // Some constants
  std::string FILENAME = "shakespeare.txt";
  std::string SEARCH_WORD = "love";

  // Get the number of processes in MPI_COMM_WORLD
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get the rank of this process in MPI_COMM_WORLD
  int my_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  MPI_Barrier(MPI_COMM_WORLD);
  double start_time = MPI_Wtime();

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
    manager(my_rank, world_size, FILENAME, SEARCH_WORD);
  }else{
    worker(my_rank, world_size, SEARCH_WORD);
  } 

  // Finalize MPI
  // This must always be called after all other MPI functions
  MPI_Barrier(MPI_COMM_WORLD);
  double total_time = MPI_Wtime() - start_time;

  std::cout << "Process: " << my_rank << ". Message: DONE" << std::endl;
  MPI_Finalize();

  if (my_rank == 0) 
  {
    std::cout << "Total time: " << total_time << " seconds\n";
  }
  return 0;
}