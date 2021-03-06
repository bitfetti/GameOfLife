#ifndef GAMEOFLIFE_H_
#define GAMEOFLIFE_H_

#include <cstdio>
#include <iostream>
#include <cstring>
#include <vector>
#include <cassert>					/* for assert() */
#include <ctime>					/* for time() */
#include <cstdlib>					/* for srand() and rand() */
#ifdef WIN32					// Windows system specific
	#include <windows.h>			/* for QueryPerformanceCounter */
#else							// Unix based system specific
	#include <sys/time.h>			/* for gettimeofday() */
#endif
#include <CL/cl.h>					/* OpenCL definitions */

#include "../inc/KernelFile.hpp"	/* for reading OpenCL kernel files */
#include "../inc/PatternFile.hpp"	/* for reading population files */

/**
* Definition of live and dead state
*/
#define ALIVE 255
#define DEAD 0

inline unsigned int countDigits(unsigned int x) {
	unsigned count=1;
	unsigned int value= 10;
	while (x>=value) { value*=10; count++; }
	return count;
}

class GameOfLife {
private:
	bool                 spawnMode;  /**< false:random mode, true:file mode */
	unsigned char           *rules;  /**< rules for calculating next generation */
	size_t          rulesSizeBytes;  /**< size of rules in bytes */
	std::string         humanRules;  /**< rules as an int, 9 separates survival/birth */
	float               population;  /**< density of live cells when using random starting population */
	PatternFile        patternFile;  /**< file when using static starting population */
	unsigned char   *startingImage;  /**< image of starting population */
	unsigned char          *imageA;  /**< first image on the host */
	unsigned char          *imageB;  /**< second image on the host */
	int               imageSize[2];  /**< width and height of image */
	size_t          imageSizeBytes;  /**< size of image in bytes */
	bool              switchImages;  /**< switch for image exchange */

	unsigned long      generations;  /**< number of calculated generations */
	int    generationsPerCopyEvent;  /**< number of executed kernels during 1 read image call */
	bool                   CPUMode;  /**< CPU/OpenCL switch for calculating next generation */
	bool                    paused;  /**< start/stop calculation of next generation */
	bool                 singleGen;  /**< switch for single generation mode */
	float            executionTime;  /**< execution time for calculation of 1 generation */
	cl_bool               readSync;  /**< switch for synchronous reading of images from device */
	
	cl_context             context;  /**< CL context */
	cl_device_id          *devices;  /**< CL device list */
	cl_command_queue  commandQueue;  /**< CL command queue */
	cl_program             program;  /**< CL program  */
	cl_kernel               kernel;  /**< CL kernel */
	std::string kernelBuildOptions;  /**< CL kernel build options */
	std::string         kernelInfo;  /**< CL kernel information */
	size_t        globalThreads[2];  /**< CL total number of work items for a kernel */
	size_t         localThreads[2];  /**< CL number of work items per group */
	
	cl_mem            deviceImageA;  /**< CL image object for first image */
	cl_mem            deviceImageB;  /**< CL image object for second image */
	size_t                rowPitch;  /**< CL row pitch for image objects */
	size_t               origin[3];  /**< CL offset for image operations */
	size_t               region[3];  /**< CL region for image operations */
	cl_mem             deviceRules;  /**< cL memory object for rules */

public:
	/** 
	* Constructor.
	* Initialize member variables
	*/
	GameOfLife():
			spawnMode(false),
			rules(NULL),
			humanRules(""),
			population(0.0f),
			startingImage(NULL),
			imageA(NULL),
			imageB(NULL),
			switchImages(true),
			generations(0),
			generationsPerCopyEvent(0),
			CPUMode(false),
			paused(true),
			singleGen(false),
			executionTime(0.0f),
			readSync(CL_TRUE),
			kernelBuildOptions(""),
			kernelInfo("")
		{
			imageSize[0] = 0;
			imageSize[1] = 0;
	}
	
	/** 
	* DeConstructor.
	* Cleanup host and device memory
	*/
	~GameOfLife() { freeMem(); }

	/**
	* Setup host/device memory and OpenCL.
	* @return 0 on success and -1 on failure
	*/
	int setup();

	/**
	* Calculate next generation.
	* @return 0 on success and -1 on failure
	*/
	int nextGeneration(unsigned char* bufferImage);
	
	/**
	* Reset the board to the starting population.
	* @return 0 on success and -1 on failure
	*/
	int resetGame(unsigned char *bufferImage);
	
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
	* Get CPU Mode.
	* @return CPUMode
	*/
	bool isCPUMode() {
		return CPUMode;
	}
	
	/**
	* Get single generation mode.
	* @return singleGen
	*/
	bool isSingleGeneration() {
		return singleGen;
	}
	
	/**
	* Get readSync.
	* @return readSync
	*/
	bool isReadSync() {
		return readSync;
	}
	
	/**
	* Get spawn mode.
	* @return spawnMode
	*/
	bool isFileMode() {
		return spawnMode;
	}
	
	/**
	* Get rule used for calculating next generations.
	* @return humanRules
	*/
	std::string getRule() {
		return humanRules;
	}
	
	/**
	* Switch to CPU/OpenCL mode
	*/
	void switchCPUMode() {
		CPUMode = !CPUMode;
		if (generations == 0) return;
		
		/* Update first OpenCL/CPU image to last calculated generation */
		if (CPUMode) {  /* Switch from OpenCL to CPU */
			cl_int status = clEnqueueReadImage(commandQueue,
				switchImages ? deviceImageA : deviceImageB,
				CL_TRUE, origin, region, rowPitch, 0,
				switchImages ? imageA : imageB,
				NULL, NULL, NULL);
			assert(status == CL_SUCCESS);
		} else {        /* Switch from CPU to OpenCL */
			cl_int status = clEnqueueWriteImage(commandQueue,
				switchImages ? deviceImageA : deviceImageB,
				CL_TRUE, origin, region, rowPitch, 0,
				switchImages ? imageA : imageB,
				NULL, NULL, NULL);
			assert(status == CL_SUCCESS);
		}
	}

	/**
	* Start/stop calculation of next generation.
	*/
	void switchPause() {
		paused = !paused;
	}

	/**
	* Switch single generation mode on/off.
	*/
	void switchSingleGeneration() {
		singleGen = !singleGen;
	}
	
	/**
	* Switch synchronous reading of images on/off.
	*/
	void switchreadSync() {
		readSync==CL_TRUE?readSync=CL_FALSE:readSync=CL_TRUE;
	}
	
	/**
	* Get execution time of current generation.
	* @return executionTime
	*/
	float getExecutionTime() {
		return executionTime;
	}
	
	/**
	* Get number of calculated generations.
	* @return generations
	*/
	unsigned long getGenerations() {
		return generations;
	}
	
	/**
	* Get number of executed kernels while
	* copying calculated next generation image
	* from device to host.
	* @return generationsPerCopyEvent
	*/
	int getGenerationsPerCopyEvent() {
		if (CPUMode)
			return 1;
		else
			return generationsPerCopyEvent;
	}

	/**
	* Get image of current generation.
	* @return image
	*/
	unsigned char * getImage() {
		return imageA;
	}
	
	/**
	* Get width of image.
	* @return imageSize[0]
	*/
	int getWidth() {
		return imageSize[0];
	}
	
	/**
	* Get height of image.
	* @return imageSize[1]
	*/
	int getHeight() {
		return imageSize[1];
	}
	
	/**
	* Get options used for building OpenCL kernel.
	* @return kernelBuildOptions
	*/
	std::string getKernelInfo() {
		return kernelInfo;
	}
	
	/**
	* Set the starting population for random mode.
	* @param _population chance to create a live cell
	*/	
	void setPopulation(float _population) {
		spawnMode = false;
		population = _population;
	}
	
	/**
	* Set the filename for file mode.
	* @param _fileName path to fileName used for starting population
	*/
	void setFilename(char *_fileName) {
		spawnMode = true;
		patternFile.setFilename(_fileName);
	}
	
	/**
	* Set the width and height of the board.
	* @param _width width
	* @param _height height
	*/
	void setSize(int _width, int _height) {
		imageSize[0] = _width;
		imageSize[1] = _height;
	}
	
	/**
	* Set the rule for calculating next generations.
	* @param _rule rule as an array of characters
	*/
	int setRule(char *_rule);
	
	/**
	* Set the OpenCL kernel work-items per work-group.
	* @param c switch for clamp mode
	* @param x work-items per work-group for x
	* @param y work-items per work-group for y
	*/
	void setKernelBuildOptions(int c, std::string x, std::string y) {
		if (c == 1) {
			/* Set clamp mode */
			kernelBuildOptions.append("-D CLAMP ");
			kernelInfo.append("clamp: on");
		} else {
			kernelInfo.append("clamp: off");
		}
		if (atoi(x.c_str()) > 0) {
			/* work-items per work-group for x */
			kernelBuildOptions.append("-D TPBX=");
			kernelBuildOptions.append(x);
			kernelBuildOptions.append(" ");
		}
		if (atoi(y.c_str()) > 0) {
			/* work-items per work-group for y */
			kernelBuildOptions.append("-D TPBY=");
			kernelBuildOptions.append(y);
		}
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
	* Read initial population from file.
	*/
	int readPopulation();

	/**
	* Spawn initial population.
	*/
	int spawnPopulation();
	
	/**
	* Spawn random population.
	*/
	int spawnRandomPopulation();
	
	/**
	* Spawn predefined population.
	*/
	int spawnStaticPopulation();

	/**
	* Calculate next generation with OpenCL.
	* @return 0 on success and -1 on failure
	*/
	int nextGenerationOpenCL(unsigned char* bufferImage);
	
	/**
	* Calculate next generation with CPU.
	* @return 0 on success and -1 on failure
	*/
	int nextGenerationCPU(unsigned char* bufferImage);
	
	/**
	* Calculate the number of neighbours for a cell.
	* @param x x coordinate of cell
	* @param y y coordinate of cell
	* @param image search in this image
	* @return number of neighbours
	*/
	int getNumberOfNeighbours(const int x, const int y, const unsigned char *image);

	/**
	* Get the state of a cell.
	* @param x and y coordinate of cell
	* @param image get state from this image
	* @return state
	*/
	inline unsigned char getState(const int x, const int y, const unsigned char *image) {
		return (image[4*x + (4*imageSize[0]*y)]);
	}
	
	/**
	* Set the state of a cell.
	* @param x x coordinate of cell
	* @param y y coordinate of cell
	* @param state new state of cell
	* @param image update state in this image
	*/
	void setState(const int x, const int y, const unsigned char state, unsigned char *image) {
		image[4*x + (4*imageSize[0]*y)] = state;
		image[(4*x+1) + (4*imageSize[0]*y)] = state;
		image[(4*x+2) + (4*imageSize[0]*y)] = state;
		image[(4*x+3) + (4*imageSize[0]*y)] = 1;
	}
};

#endif
