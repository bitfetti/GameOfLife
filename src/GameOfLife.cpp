#include "../inc/GameOfLife.hpp"
using namespace std;

int GameOfLife::setup() {
	if(setupHost()!=0)
		return -1;

	if(setupDevice()!=0)
		return -1;

	return 0;
}

int GameOfLife::setupHost() {
	image = (int *) malloc(imageSizeBytes);
	if(image == NULL)
		return -1;

	/* Spawn initial population */
	if(spawnPopulation()!=0)
		return -1;

	return 0;
}

int GameOfLife::spawnPopulation() {
	cout << "Spawning population for Game of Life with a chance of " << population << endl;
	cout << "-------------------------------------------" << endl;

	int random, x, y;

	srand(time(NULL));
	for (x=0;x<width;x++) {
		for (y=0;y<height;y++) {
			random=rand() % 100;
			if ((float)random/100.0f > population)
				setState(x,y,0);
			else
				setState(x,y,1);
		}
	}

	return 0;
}

int GameOfLife::getState(int x, int y) {
	return (image[x + (width*y)]);
}

void GameOfLife::setState(int x, int y, int state) {
	image[x + (width*y)] = state;
}

int GameOfLife::setupDevice(void) {
	cl_int status = CL_SUCCESS;
	const char* kernelFile = "GameOfLife_Kernels.cl";
	KernelFile kernels;

	try {
		/* Get platform information for a NVIDIA or ATI GPU */
		cl::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		cl::vector<cl::Platform>::iterator pit;

		if(platforms.size() > 0) {
			for(pit = platforms.begin(); pit != platforms.end(); pit++) {
				if(!strcmp((*pit).getInfo<CL_PLATFORM_VENDOR>().c_str(), "Advanced Micro Devices, Inc.")
					|| !strcmp((*pit).getInfo<CL_PLATFORM_VENDOR>().c_str(), "NVIDIA Corporation")) {
						break;
				}
			}
		}

		/* 
		* If there is a NVIDIA or ATI GPU, use it,
		* else pass a NULL and get whatever the implementation thinks we should be using
		*/
		cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(*pit)(), 0 };

		/* Creating a context for a NVIDIA/AMD platform */
		context = cl::Context(CL_DEVICE_TYPE_GPU, cps, NULL, NULL, &status); assert(status == CL_SUCCESS);

		/* Get the list of GPU devices associated with context */
		devices = context.getInfo<CL_CONTEXT_DEVICES>();
		//assert(devices.size() > 0);

		/* Create command queue */
		commandQueue = cl::CommandQueue(context, devices[0], 0, &status); assert(status == CL_SUCCESS);

		/* Allocate the OpenCL buffer memory objects for the images on the device GMEM */
		deviceImageA = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, imageSizeBytes, image, &status);
		assert(status == CL_SUCCESS);
		deviceImageB = cl::Buffer(context, CL_MEM_WRITE_ONLY, imageSizeBytes, NULL, &status);
		assert(status == CL_SUCCESS);

		/* Read in the OpenCL kernel from the source file */
		if (!kernels.open(kernelFile)) {
			cerr << "Couldn't load CL source code" << endl;
			return -1;
		}
		cl::Program::Sources sources(1, make_pair(kernels.source().data(), kernels.source().size()));
		program = cl::Program(context, sources);

		/* Create a cl program executable for all the devices specified */
		program.build(devices);

		/* Get a kernel object handle for a kernel with the given name */
		kernel = cl::Kernel(program, "nextGeneration", &status); assert(status == CL_SUCCESS);

		/* Check group size against kernelWorkGroupSize */
		status = kernel.getWorkGroupInfo(devices[0], CL_KERNEL_WORK_GROUP_SIZE, &kernelWorkGroupSize);
		assert(status == CL_SUCCESS);

		if((cl_uint)(sizeX*sizeY) > kernelWorkGroupSize) {
			cerr<<"Out of Resources!"<<endl;
			cerr<<"Group Size specified : "<<sizeX*sizeY<<endl;
			cerr<<"Max Group Size supported on the kernel : "<<kernelWorkGroupSize<<endl;
			return -1;
		}

		return 0;
	} catch (cl::Error err) {
		cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << endl;
		/* Get the build log for the first device */
		/*try {
			program.getBuildInfo(devices[0], CL_PROGRAM_BUILD_STATUS, &status);
			cerr << "\nRetrieving build log\n"	<< status << endl;
		} catch (cl::Error err) {
			cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << endl;
		}*/

		return -1;
	}
}

int GameOfLife::nextGeneration(void) {
	cl_int status = CL_SUCCESS;

	try {
		/* Set arguments to the kernel */
		kernel.setArg(0,sizeof(cl_mem),(void*) &deviceImageA);		
		kernel.setArg(1,sizeof(cl_mem),(void*) &deviceImageB);
		kernel.setArg(2,sizeof(int),(void*) &width);
		kernel.setArg(3,sizeof(int),(void*) &height);

		/* Enqueue a kernel run call and wait for kernel to finish */
		cl::KernelFunctor func = kernel.bind(commandQueue,
			cl::NDRange(globalSizeX,globalSizeY), cl::NDRange(sizeX,sizeY));
		func().wait();

		/* Synchronous (i.e. blocking) read of results */
		commandQueue.enqueueReadBuffer(deviceImageB, CL_TRUE, 0, imageSizeBytes, image, NULL, NULL);

		return 0;
	} catch (cl::Error err) {
		cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << endl;
		return -1;
	}
}

int GameOfLife::freeMem() {
	/* Releases OpenCL resources */
	//kernel.release();
	//program.release();
	//outputBuffer.release();
	//commandQueue.release();
	//context.release();

	/* Release program resources */
	if(image)
		free(image);

	return 0;
}