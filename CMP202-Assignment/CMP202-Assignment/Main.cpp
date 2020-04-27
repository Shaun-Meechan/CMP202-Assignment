#include <list>
#include <complex>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <queue>
#include <ppl.h>
#include <chrono>
#include <iostream>

//Import what we need from the standard library
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::cout;
using std::endl;
using std::mutex;
using std::vector;
using std::thread;
using std::condition_variable;
using std::unique_lock;
using std::list;
using std::ofstream;
using std::complex;
using namespace concurrency;

//Define the name "the_clock" for the clock we are using
typedef std::chrono::steady_clock the_clock;

//Define the threads
vector<thread*> workerThreads;
//Create global variables
//If true program will begin to shutdown
bool finish = false;
//Mutex for our worker threads
mutex workerThreadMutex;
//Mutex to protect our global thread arguments
mutex threadArgsMutex;
//mutex for our image writer thread
mutex imageWriterMutex;
//Bool to put worker threads to sleep
bool noMoreTasks = false;
//bool to start the image writer
bool makeTGA = false;
//bool to wake worker
bool wakeWorker = false;
//Condition variable to add or remove worker threads
condition_variable workerCV;
//Condition variable to start our image writer thread
condition_variable imageWriterCV;
//Used to make sure that the program closes correctly we asked to do so
bool killSwitch = false;
//Function to see if we can change the status of finish
bool changeFinish();
//How many worker threads to make. Used in main
int workerThreadsToMake = 0;
//Define how many sections we want to make
int sectionsToMake = 0;
// The size of the image to generate.
const int WIDTH = 1920;
const int HEIGHT = 1200;
// The number of times to iterate before we assume that a point isn't in the
// Mandelbrot set.
// (You may need to turn this up if you zoom further into the set.)
const int MAX_ITERATIONS = 500;
// The image data.
// Each pixel is represented as 0xRRGGBB.
uint32_t image[HEIGHT][WIDTH];

// Write the image to a TGA file with the given name.
// Format specification: http://www.gamers.org/dEngine/quake3/TGA.txt
void write_tga(const char* filename)
{
	while (finish != true)
	{
		//Put the thread to sleep
		unique_lock<mutex> TGALock(imageWriterMutex);
		imageWriterCV.wait(TGALock, []() {return makeTGA; });
		ofstream outfile(filename, ofstream::binary);

		uint8_t header[18] = {
			0, // no image ID
			0, // no colour map
			2, // uncompressed 24-bit image
			0, 0, 0, 0, 0, // empty colour map specification
			0, 0, // X origin
			0, 0, // Y origin
			WIDTH & 0xFF, (WIDTH >> 8) & 0xFF, // width
			HEIGHT & 0xFF, (HEIGHT >> 8) & 0xFF, // height
			24, // bits per pixel
			0, // image descriptor
		};
		outfile.write((const char*)header, 18);

		for (int y = 0; y < HEIGHT; ++y)
		{
			for (int x = 0; x < WIDTH; ++x)
			{
				uint8_t pixel[3] = {
					image[y][x] & 0xFF, // blue channel
					(image[y][x] >> 8) & 0xFF, // green channel
					(image[y][x] >> 16) & 0xFF, // red channel
				};
				outfile.write((const char*)pixel, 3);
			}
		}

		outfile.close();
		if (!outfile)
		{
			// An error has occurred at some point since we opened the file.
			cout << "Error writing to " << filename << endl;
			exit(1);
		}
	}
}

void compute_mandelbrot(double left, double right, double top, double bottom, int startY, int endY)
{
	for (int y = startY; y < endY; ++y)
	{
		for (int x = 0; x < WIDTH; ++x)
		{
			// Work out the point in the complex plane that
			// corresponds to this pixel in the output image.
			complex<double> c(left + (x * (right - left) / WIDTH),
				top + (y * (bottom - top) / HEIGHT));

			// Start off z at (0, 0).
			complex<double> z(0.0, 0.0);

			// Iterate z = z^2 + c until z moves more than 2 units
			// away from (0, 0), or we've iterated too many times.
			int iterations = 0;
			while (abs(z) < 2.0 && iterations < MAX_ITERATIONS)
			{
				z = (z * z) + c;

				++iterations;
			}

			if (iterations == MAX_ITERATIONS)
			{
				// z didn't escape from the circle.
				// This point is in the Mandelbrot set.
				image[y][x] = 0x000000; // black
			}
			else
			{
				// z escaped within less than MAX_ITERATIONS
				// iterations. This point isn't in the set.
				if (iterations > 150)
				{
					image[y][x] = 0xffffff; // white
				}
				if (iterations <= 50)
				{
					image[y][x] = 0xadcc10; // yellow
				}
				if (iterations > 50 && iterations <= 100)
				{
					image[y][x] = 0x20d11d; // light green
				}
				if (iterations > 100 && iterations <= 150)
				{
					image[y][x] = 0xba1a3a; // light pink
				}

			}
		}
	}
}

//Create a structure that contains all the data we need for our compute mandlebrot thread.
struct threadArgs
{
	double left = -2.0;
	double right = 1.0;
	double top = 1.125;
	double bottom = -1.125;
	short unsigned int startY = 0;
	short unsigned int endY = 0;
	short unsigned int loopCounter = 1;
};

//Create a variable to use the structure
threadArgs globalThreadArgs;

//Function that handles working on tasks. Used by worker threads
void workerThreadFunction()
{
	cout << "Started a worker thread!" << endl;
	//Begin working
	while (finish == false)
	{
		unique_lock<mutex> workerLock(workerThreadMutex);

		threadArgsMutex.lock();
		if (globalThreadArgs.loopCounter >= 17 || globalThreadArgs.loopCounter > sectionsToMake)
		{
			threadArgsMutex.unlock();
			noMoreTasks = true;
			workerCV.wait(workerLock, []() {return wakeWorker; });
			break;
		}
		threadArgsMutex.unlock();
		//If no more tasks is true then we stop the thread.
		if (noMoreTasks == true)
		{
			cout << "Worker was told to stop." << endl;
			break;
		}

		cout << "Worker thread starting work.\n";
		
		//Lock the gloabal structure, increment the global counter then unlock.
		threadArgsMutex.lock();
		threadArgs localThreadArgs = globalThreadArgs;
		globalThreadArgs.loopCounter += 1;
		threadArgsMutex.unlock();
		localThreadArgs.startY = (75 * localThreadArgs.loopCounter) - 75;
		localThreadArgs.endY = (75 * localThreadArgs.loopCounter);
		the_clock::time_point start = the_clock::now();

		//currentTask->run();
		compute_mandelbrot(localThreadArgs.left,localThreadArgs.right,localThreadArgs.top,localThreadArgs.bottom,localThreadArgs.startY,localThreadArgs.endY);
		//Timing code.
		the_clock::time_point end = the_clock::now();
		auto time_taken = duration_cast<milliseconds>(end - start).count();
		std::cout << "Time taken was " << time_taken << std::endl;
		cout << "Finished task!" << endl;
	}
}

//Function for stopping worker threads and starting the queue filler thread
void threadManager()
{
	while (finish != true)
	{
		//Don't do anything for the first 10 seconds.
		std::this_thread::sleep_for(std::chrono::seconds(15));
		//Lock both mutexes as we will be using them
		std::unique_lock<mutex> TGALock(imageWriterMutex);

		//If the worker thhreads are still working, sleep.
		while (noMoreTasks == false)
		{
			std::this_thread::sleep_for(std::chrono::seconds(15));
		}

		//If the worker threads are finished write the image and ask the user to end the program
		if (globalThreadArgs.loopCounter >= 17 || globalThreadArgs.loopCounter >= sectionsToMake)
		{
			wakeWorker = true;
 			makeTGA = true;
			imageWriterCV.notify_all();
			cout << "Image finished. Writing to file.\n";
			cout << "File created.\n";
			cout << "Please enter 's' to end the program.\n";
		}
	}
}

//Check to see if we can change the status of finish based on killSwitch value
//If kill switch equals true finish state cannot be changed and all threads must be stopped.
//If kill switch equals false then the value of false can be changed.
bool changeFinish()
{
	if (killSwitch == true)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int main()
{
	std::string value = "";
	bool validData = false;

	//Make sure the user enters a valid number of therads.
	do
	{
		cout << "How many worker threads do you want?\n";
		std::cin >> workerThreadsToMake;
		if (std::cin.good())
		{
			validData = true;
		}
		else
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			cout << "ERROR: Invalid input, please try again" << endl;
		}
	} while (validData == false);

	//Make sure the user enters a valid number of sections.
	do
	{
		cout << "How many sections do you want?\n";
		std::cin >> sectionsToMake;
		if (std::cin.good())
		{
			validData = true;
		}
		else
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			cout << "ERROR: Invalid input, please try again" << endl;
		}
	} while (validData == false);
	//Start all our worker threads
	for (int i = 0; i < workerThreadsToMake; i++)
	{
		workerThreads.push_back(new thread(workerThreadFunction));
	}
	
	//Create our image writer and thread manager threads.
	thread threadManagerThread(threadManager);
	thread imageWriterThread(write_tga,"output.tga");

	cout << "Enter 's' at any time to end the program" << endl;
	
	//Let the user enters commands
	while (finish == false)
	{
		std::cin >> value;

		if (value == "s")
		{
			killSwitch = true;
			finish = true;
			cout << "Finishing threads. This should take 50 seconds at most." << endl;
		}
		//Debug commands
		if (value == "getWorkerThreads")
		{
			cout << "Worker threads count: " << workerThreads.size() << endl;
		}
	}

	//Start to join all threads and close the program
	threadManagerThread.join();
	cout << "Thread manager stopped.\n";

	unique_lock<mutex> TGALock(imageWriterMutex);
	makeTGA = true;
	imageWriterCV.notify_one();
	imageWriterThread.join();
	cout << "Image writer thread stopped.\n";

	unique_lock<mutex> workerLock(workerThreadMutex);
	workerLock.unlock();
	wakeWorker = true;
	workerCV.notify_all();
	for (unsigned int i = 0; i < workerThreads.size(); i++)
	{
		workerThreads[i]->join();
	}
	cout << "All worker threads stopped." << endl;

	cout << "Program finished!";
	return 0;
}