#include<pipelined.hh>
#include<sgemv.hh>
#include<stdio.h>

using namespace pipelined;

void test_sgemv(u32 m, u32 n)
{
    pipelined::zeromem();

    const uint32_t M = m;
    const uint32_t N = n;

    const uint32_t Y = 0;
    const uint32_t X = Y + M*sizeof(float);
    const uint32_t A = X + N*sizeof(float);

    for (uint32_t i=0; i<M; i++) *((float*)(pipelined::MEM.data() + Y + i*sizeof(float))) = 0.0;
    for (uint32_t j=0; j<N; j++) *((float*)(pipelined::MEM.data() + X + j*sizeof(float))) = (float)j;
    for (uint32_t i=0; i<M; i++) for (uint32_t j=0; j<N; j++) *((float*)(pipelined::MEM.data() + A + (i+M*j)*sizeof(float))) = (float)i;

    pipelined::zeroctrs();

    pipelined::GPR[3].data() = Y;
    pipelined::GPR[4].data() = A;
    pipelined::GPR[5].data() = X;
    pipelined::GPR[6].data() = M;
    pipelined::GPR[7].data() = N;
    pipelined::GPR[8].data() = M;
    
    pipelined::sgemv((float*)(pipelined::MEM.data() + Y),(float*)(pipelined::MEM.data() + A), (float*)(pipelined::MEM.data() + X), M, N, M);

    pipelined::caches::L2.flush();
    pipelined::caches::L3.flush();
    
    if (pipelined::tracing) printf("\n");
    printf("M = %4d, N = %4d : instr = %6lu, cyc = %8lu, L1D(access= %6lu, hit = %6lu, miss = %6lu), L2(miss = %6lu), L3(miss = %6lu) | ",
	    M, N, pipelined::counters::operations, pipelined::counters::cycles, pipelined::caches::L1D.accesses, pipelined::caches::L1D.hits, pipelined::caches::L1D.misses,
	    pipelined::caches::L2.misses, pipelined::caches::L3.misses);
    bool pass = true;
    for (uint32_t i=0; i<M; i++)
    {
	float y = *((float*)(pipelined::MEM.data() + Y + i*sizeof(float)));
	// printf("\ny[%2d] = %10.0f", i, y);
	if (y != ((N*(N-1))/2)*i) { pass = false; }
    }
    // printf("\n");
    if (pass) printf("PASS\n");
    else      printf("FAIL\n");
}

int main
(
    int		  argc,
    char	**argv
)
{
    printf("L1D: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n",
	   pipelined::caches::L1D.capacity(), pipelined::caches::L1D.nsets(), pipelined::caches::L1D.nways(), pipelined::caches::L1D.linesize());
    printf("L1I: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n",
	   pipelined::caches::L1I.capacity(), pipelined::caches::L1I.nsets(), pipelined::caches::L1I.nways(), pipelined::caches::L1I.linesize());
    printf("L2: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n",
	   pipelined::caches::L2.capacity(), pipelined::caches::L2.nsets(), pipelined::caches::L2.nways(), pipelined::caches::L2.linesize());
    printf("L3: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n",
	   pipelined::caches::L3.capacity(), pipelined::caches::L3.nsets(), pipelined::caches::L3.nways(), pipelined::caches::L3.linesize());

    for (uint32_t m = 4; m <= 64; m *= 2) for (uint32_t n = m; n <= 2*m; n *= 2)
    {
	test_sgemv(m,n);
    }
    
    for (uint32_t m = 4; m <= 8; m *= 2) for (uint32_t n = m; n <= 1024; n *= 2)
    {
	test_sgemv(m,n);
    }
    
    return 0;
}
