// C++
#include <iostream>
#include <string>
#include <fstream>
#include <string>
#include <cassert>

#include <unistd.h>

// ACL specific includes
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"
using namespace aocl_utils;
using std::cout;
using std::string;
using std::endl;

#define KERNEL_PATH "bin/isat"
#define KERNEL_NAME "dLuSolver"
#define CNF_PATH "bin/formula.cnf"
#define MAX_WIDTH 3
#define VARS 20
// MAX WORK ITENS 2147483647

//Atributos do dispositivo
static cl_platform_id platform = NULL;
scoped_array<cl_device_id> device;

static cl_command_queue queue   = NULL;
static cl_kernel        kernel  = NULL;
static cl_program       program = NULL;
static cl_context       context = NULL;

static cl_int status = 0;

static uint d_numberof_clauses = 0;

// Atributos da formula
static size_t d_clauses_size = 0;

static cl_short  *d_clauses = NULL;
static cl_ulong *d_result = NULL;

inline void cleanup();
inline void harpSetup();
inline void setupBuffers();
inline void launchKernel();
inline void showResult(const double time);
inline void parse();
