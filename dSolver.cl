#define MAX_VARS 20
#define UNSAT 0x8000000000000000
#define SINAL_BIT 0x80000000
#define CLAUSES 2000
#define MAX_SIZE 3*CLAUSES

typedef unsigned long long int uint64_t;
typedef long long int int64_t;

bool dEvaluate(const short short int *clauses, uint64_t d_clauses_size, uint64_t assignment)
{
	for (uint64_t i = 0; i < d_clauses_size; i += 3) {

		// isso na fpga é indexação clauses[7]
		unsigned short short int sinal1 = (clauses[i] & SINAL_BIT) == 0 ? 0 : 1;

		// isso na fpga é indexação clauses[7];
		unsigned short short int sinal2 = (clauses[i + 1] & SINAL_BIT) == 0 ? 0 : 1;

		// isso na fpga é indexação clauses[7];
		unsigned short short int sinal3 = (clauses[i + 2] & SINAL_BIT) == 0 ? 0 : 1;

		if ((sinal1 ^ ((assignment >> (abs(clauses[i]) - 1)) & 1L)) || (sinal2 ^ ((assignment >> (abs(clauses[i + 1]) - 1)) & 1L)) || (sinal3 ^ ((assignment >> (abs(clauses[i + 2]) - 1)) & 1L)))
			continue;
		else
			return false;

	}
	return true;
}

__kernel __attribute__((task))
void dLuSolver(__global short short int *restrict d_clauses, __global int64_t *restrict d_result, const uint64_t d_clauses_size)
{
	*d_result = -2;
	short short int clauses[MAX_SIZE];

	for (size_t i = 0; i < d_clauses_size; i++)
		clauses[i] = d_clauses[i];

	for (uint64_t assignment = 0; assignment < (1UL << MAX_VARS); assignment++)
		if (dEvaluate(clauses, d_clauses_size, assignment))
		{
			*d_result = assignment;
			return;
		}

	*d_result = -1;
	return;
}

