#ifndef GAMEOFLIFE_H_
#define GAMEOFLIFE_H_

#include <cstdio>
#include <iostream>
#include <fstream>
#include <cassert>       /* for assert() */
#include <ctime>         /* for time() */
#include <cstdlib>       /* for srand() and rand() */
#define __CL_ENABLE_EXCEPTIONS
#define __NO_STD_VECTOR
#define __NO_STD_STRING
#include <CL/cl_fixed.hpp>
//#include <CL/cl_gl.h>    /* OpenCL/OpenGL interoperation */
#include "../inc/KernelFile.hpp"

class GameOfLife {
private:
	float               population;       /**< starting population density */
	int                      width;       /**< width of image */
	int                     height;       /**< height of image */
	size_t          imageSizeBytes;       /**< size of image in bytes */
	unsigned char    *nextGenImage;       /**< temp-image for CPU calculation */

	bool                    paused;       /**< start/stop calculation of next generation */

	cl::Context            context;       /**< CL context */
	cl::vector<cl::Device> devices;       /**< CL device list */
	//cl::Buffer        deviceImageA;       /**< CL memory buffer for first image on the device */
	//cl::Buffer        deviceImageB;       /**< CL memory buffer for second image on the device */
	cl::Image2D       deviceImageA;       /**< CL memory buffer for first image on the device */
	cl::Image2D       deviceImageB;       /**< CL memory buffer for second image on the device */
	cl::CommandQueue  commandQueue;       /**< CL command queue */
	cl::Program            program;       /**< CL program  */
	cl::Kernel              kernel;       /**< CL kernel */

public:
	bool                 useOpenCL;       /**< use OpenCL for calculation of next generation */
	unsigned char           *image;       /**< image on the host that is displayed with OpenGL */

public:
	/** 
	* Constructor.
	* Initialize member variables
	*/
	GameOfLife(float p, int w, int h)
		: population(p), width(w), height(h), image(NULL),
		nextGenImage(NULL),	useOpenCL(true), paused(true) {
			imageSizeBytes = w*h*sizeof(char);
	}

	/**
	* Setup host/device memory and OpenCL.
	* @return 0 on success and -1 on failure
	*/
	int setup();

	/**
	* Calculate next generation.
	* @return 0 on success and -1 on failure
	*/
	int nextGeneration();

	/**
	* Get the state of a cell.
	* @return state
	*/
	unsigned char getState(int x, int y) {
		return (image[x + (width*y)]);
	}

	/**
	* Free memory.
	* @return 0 on success and -1 on failure
	*/
	int freeMem();

	/**
	* Get running state of GameOfLife.
	* @return paused
	*/
	bool isPaused() {
		return paused;
	}

	/**
	* Start/stop calculation of next generation.
	*/
	void pause() {
		paused = !paused;
		std::cout << (paused ? "Stopping ":"Starting ")
				<< "calculation of next generation." << std::endl;
	}

private:

	/**
	* Host initialisations.
	* Allocate and initialize host image
	* @return 0 on success and -1 on failure
	*/
	int setupHost();

	/**
	* Device initialisations.
	* Set up context, device list, command queue, memory buffers
	* Allocate device images
	* Build CL kernel program executable
	* @return 0 on success and -1 on failure
	*/
	int setupDevice();

	/**
	* Spawn initial population.
	* @return 0 on success and -1 on failure
	*/
	int spawnPopulation();

	/**
	* Calculate next generation with CPU.
	* @return 0 on success and -1 on failure
	*/
	int nextGenerationCPU();


	/**
	* Calculate next generation with OpenCL.
	* @return 0 on success and -1 on failure
	*/
	int nextGenerationOpenCL();

	/**
	* Set the state of a cell.
	*/
	void setState(int x, int y, unsigned char state, unsigned char *image) {
		image[x + (width*y)] = state;
	}

	/**
	* Calculate the number of neighbours for a cell.
	* @return number of neighbours
	*/
	int getNumberOfNeighbours(const int x, const int y);
};

#endif
