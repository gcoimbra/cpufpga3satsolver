#include "Harp.h"
double time_start,time_end;
int main()
{
	harpSetup();

	setupBuffers();
	launchKernel();

	showResult(time_end - time_start);

	cleanup();
}

void toBinary(const uint64_t c)
{
	for (int i = VARS; i >= 0; --i)
		putchar( (c & (1 << i)) ? '1' : '0' );
	putchar('\n');
}

// Mostra resultados
inline void showResult(const double time)
{
	assert(time > 0);
	printf("Time: %.4f us\n", (time)*1000000.0f);
	cout << "SAT? ";
	printf("%ld\n", *d_result);
	toBinary(*d_result);

	for (size_t i = 0; i < d_numberof_clauses * 3; i++)
		cout << d_clauses[i] << " ";
	cout << endl;
}

// Pega primeiros atributos da formula
inline void parseAux()
{
	int64_t literal;
	FILE *formula_file;

	formula_file = fopen(CNF_PATH, "r");
	if (formula_file)
	{
		// Primeira linha
		while ((literal = getc(formula_file)) != EOF)
		{
			if(literal == 'p')
			{
				char waste[5];
				uint uwaste;

        // Pega char 'p'
				fscanf(formula_file,"%s",waste);

				// Pega quantidade de variáveis
				fscanf(formula_file,"%u",&uwaste);
				if(uwaste != VARS)
				{
					cout << "parseAux(): ERRO numero de variáveis errado:" <<  uwaste << endl;
					exit(-1);
				}

        // Pega quantidade de clausulas
				fscanf(formula_file,"%u",&d_numberof_clauses);
				if(d_numberof_clauses <= 0)
				{
					cout << "parseAux(): ERRO FATAL numero de clausulas errado:" <<  d_numberof_clauses << endl;
					exit(-1);
				}

				break;
			}
		}
		// Pega o resto do problema
		size_t x = 0, y = 0;
		do
		{
			fscanf(formula_file,"%ld",&literal);
			if(literal == 0)
			{
				x++;
				y = 0;
			}
			else
			{
				y++;
				if(abs(literal) > VARS)
				{
					cout << "parse(): ERRO FATAL variável na linha" << x << "está com valor errado:" << literal << endl;
					// exit(-1);
				}
			}
		}
		while (fgetc(formula_file) != EOF);

    // Seta tamanho da fórmula
		d_clauses_size = d_numberof_clauses * 3;

		fclose(formula_file);
	}
	else
	{
		perror("parse(): Impossível abrir arquivo!");
		exit(-1);
	}
}

// Parseia todas formulas
inline void parse()
{
	int64_t literal;
	FILE *formula_file;

	parseAux();
	// <Alloc sizes>
	d_clauses = (cl_short *) clSVMAllocAltera(context, 0, sizeof(cl_short) * d_clauses_size, 0);
	d_result = (cl_ulong *) clSVMAllocAltera(context, 0, sizeof(cl_ulong), 0);

	formula_file = fopen(CNF_PATH, "r");
	if (formula_file)
	{
		// Primeira linha
		while ((literal = getc(formula_file)) != EOF)
		{
			if(literal == 'p')
			{
				char waste[5];
				uint uwaste;

        // Pode jogar valores fora. Já foram parseados
				fscanf(formula_file,"%s",waste);
				fscanf(formula_file,"%u",&uwaste);
				fscanf(formula_file,"%u",&uwaste);

				break;
			}
		}
		// Pega o resto do problema
		size_t x = 0, y = -1;
		do
		{
			fscanf(formula_file,"%ld",&literal);
			if(literal == 0)
			{
				x++;
				cout << endl;
				y = -1;
			}
			else
			{
				y++;
				d_clauses[x * MAX_WIDTH + y] = literal;
				cout << literal << " ";
			}
		}
		while (fgetc(formula_file) != EOF);

		fclose(formula_file);
	}
	else
	{
		puts("parse(): Impossível abrir arquivo!");
		exit(-1);
	}
}

//Limpa os recursos alocados. Nome cleanup é obrigatório.
inline void cleanup()
{
	cout << "harp_setup(): Limpando..." << endl;
	if(queue)
		clReleaseCommandQueue(queue);
	if(program)
		clReleaseProgram(program);
	if(kernel)
		clReleaseKernel(kernel);
	if(d_clauses)
		clSVMFreeAltera(context,d_clauses);
	if(d_result)
		clSVMFreeAltera(context,d_result);
	if(context)
		clReleaseContext(context);
};

//Inicializa dispositivo
inline void harpSetup()
{
	cout << "harp_setup(): Iniciando setup..." << endl;
	cl_uint n_devices;

	//Pega ID da plataforma
	platform = findPlatform("Altera");
	if(platform == NULL)
	{
		cout << "Unable to get platforms..." << endl;
		cleanup();
	}

	// Get devices quantity
	device.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &n_devices));

	// We can't have more than one
	if(n_devices != 1)
	{
		cout << "Invalid number of OpenCL devices: " << n_devices << endl;
		cleanup();
	}
	cout << "Using platform: " << getPlatformName(platform).c_str() << endl;

	//Cria contexto
	cout << "harp_setup(): Creating context... " << endl;
	context = clCreateContext(0, n_devices, device, NULL, NULL, &status);
	checkError(status,"setup() Failed clCreateContext.");

	// Inicializa queue
	cout << "harp_setup(): Starting queue... " << endl;
	queue = clCreateCommandQueue(context, device[0], CL_QUEUE_PROFILING_ENABLE, &status);
	checkError(status,"setup(): Failed clCreateCommandQueue");

	//Tenta carregar o arquivo binário
	cout << "harp_setup(): Loading kernel binary file... " << endl;
	string kernelFile = getBoardBinaryFile(KERNEL_PATH, device[0]);
	cout << "Usando kernel: " << kernelFile.c_str() << endl;

	//Tenta criar o programa
	cout << "harp_setup(): Creating program... " << endl;
	program = createProgramFromBinary(context, kernelFile.c_str(), device, n_devices);

	//Começa o build
	cout << "harp_setup(): Starting build... " << endl;
	status = clBuildProgram(program,0,NULL,"",NULL,NULL);
	checkError(status,"setup(): Failed to build the program!");

	//Tries to create kernel
	cout << "harp_setup(): Creating kernel... " << endl;
	kernel = clCreateKernel(program,KERNEL_NAME,&(status));
	checkError(status,"setup(): Failed to create kernel!");
}

inline void setupBuffers()
{
	cout << "setup_buffers(): Inicializando memória compartilhada..." << endl;

	//Initialize buffers
	parse();

	cout << "setup_buffers(): Tamanho dos buffers:" << " " << d_clauses_size << " " << MAX_WIDTH << endl;
	if(d_clauses == NULL || d_result == NULL)
	{
		cout << "setup_buffers(): Failed to set args..." << endl << "d_clauses: "<< d_clauses << " d_result:" <<  d_result << " " << endl;
		cleanup();
	}

	// Set kernel arguments
  // Buffers
	cout << "setup_buffers(): Setting up kernel args..." << endl;
	status = clSetKernelArgSVMPointerAltera(kernel, 0, (void *)d_clauses);
	checkError(status,"setup_buffers(): Failed set arg 0.");
	status = clSetKernelArgSVMPointerAltera(kernel, 1, (void *)d_result);
	checkError(status,"setup_buffers(): Failed set arg 1.");

  // Settando vars
	status = clSetKernelArg(kernel, 2, sizeof(size_t),(void *)&d_clauses_size);
	checkError(status,"setup_buffers(): Failed set arg 2.");
}

inline void launchKernel()
{
	// Mapping kernel args
	// Buffers
	cout << "setup_buffers(): Mapping kernel arguments..." << endl;
	status = clEnqueueSVMMap(queue, CL_TRUE, CL_MAP_READ, (void *)d_clauses, sizeof(cl_int) * d_clauses_size, 0, NULL, NULL);
	checkError(status,"setup_buffers(): Failed clEnqueueSVMMap d_clauses");

	status = clEnqueueSVMMap(queue, CL_TRUE, CL_MAP_WRITE, (void *)d_result, sizeof(cl_ulong), 0, NULL, NULL);
	checkError(status,"setup_buffers(): Failed clEnqueueSVMMap d_result");

	// Kernel launch
	time_start = getCurrentTimestamp();
	cout << "launch_kernel):Launching the kernel..." << endl;
	status = clEnqueueTask(queue, kernel, 0, NULL, NULL);
	checkError(status,"launch_kernel()): Failed to launch kernel.");

	 		
	clFinish(queue);
	time_end = getCurrentTimestamp();
	// Unmapping kernel args
	// Buffers
	// sleep(3);
	cout << "setup_buffers(): Unmapping kernel arguments..." << endl;
	status = clEnqueueSVMUnmap(queue, d_clauses, 0, NULL, NULL);
	checkError(status,"setup_buffers(): Failed clEnqueueSVMUnmap d_clauses");

	status = clEnqueueSVMUnmap(queue, d_result, 0, NULL, NULL);
	checkError(status,"setup_buffers(): Failed clEnqueueSVMUnmap d_result");
	
	cout << "clFinish queue..." << endl;

}
