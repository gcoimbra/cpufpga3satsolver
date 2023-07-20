#define C_UNITS 8
#define MAX_VARS 63
#define UNSAT 0x8000000000000000
#define SINAL_BIT 0x80000000
#define CLAUSES 63
#define MAX_SIZE 3*CLAUSES


typedef unsigned long long int uint64_t;
typedef long long int int64_t;
__attribute__((max_global_work_dim(0)))
__kernel void reader(global int *data_in, int size)
{
	for (i = 0; i < size; ++i)
		write_channel_altera(chain_channels[0], data_in[i]);
}

bool dEvaluate(const int *clauses, uint64_t d_clauses_size, uint64_t assignment)
{
	for (uint64_t i = 0; i < d_clauses_size; i += 3) {

		//abs |clauses[i]|
		int lit1 = abs(clauses[i]);

		// isso na fpga é indexação clauses[7]
		int sinal1 = (clauses[i] & SINAL_BIT) == 0 ? 0 : 1;
		lit1 = lit1 - 1;

		//abs |clauses[i]|
		int lit2 = abs(clauses[i + 1]);

		// isso na fpga é indexação clauses[7];
		int sinal2 = (clauses[i + 1] & SINAL_BIT) == 0 ? 0 : 1;
		lit2 = lit2 - 1;

		//abs |clauses[i]|
		int lit3 = abs(clauses[i + 2]);

		// isso na fpga é indexação clauses[7];
		int sinal3 = (clauses[i + 2] & SINAL_BIT) == 0 ? 0 : 1;
		lit3 = lit3 - 1;

		if ((sinal1 ^ ((assignment >> lit1) & 1L)) || (sinal2 ^ ((assignment >> lit2) & 1L)) || (sinal3 ^ ((assignment >> lit3) & 1L)))
			continue;
		else
			return false;

	}
	return true;
}

__attribute__((max_global_work_dim(0)))
__attribute__((autorun))
__attribute__((num_compute_units(C_UNITS)))
__kernel __attribute__((task))
void solver()
{
	*d_result = -2;
	int clauses[MAX_SIZE];
	for (size_t i = 0; i < d_clauses_size; i++)
		clauses[i] = d_clauses[i];

	size_t gid = get_compute_id(0);
	uint64_t space = ceil( ((1UL) << MAX_VARS) / C_UNITS);
	uint64_t begin = gid * space;
	uint64_t end = (gid+1) * space;

	for (uint64_t assignment = begin; assignment < end; assignment++)
		if (dEvaluate(clauses, d_clauses_size, assignment))
		{
			*d_result = assignment;
			return;
		}

	*d_result = -1;
	return;
}
