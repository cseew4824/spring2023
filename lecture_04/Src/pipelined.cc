#include<pipelined.hh>

namespace pipelined
{
    const u32	params::MEM::N = 65536;
    const u32 	params::MEM::latency = 300;
    const u32 	params::L1::latency = 2;
    const u32	params::L1::nsets = 16;
    const u32 	params::L1::nways = 4;
    const u32	params::L1::linesize = 8;
    const u32	params::GPR::N = 16;
    const u32 	params::FPR::N = 8;

    std::vector<u8>		MEM(params::MEM::N);
    std::vector<reg<u32>>	GPR(params::GPR::N);
    std::vector<reg<double>>	FPR(params::FPR::N);

    units::unit			units::LDU;
    units::unit			units::STU;
    units::unit			units::FXU;
    units::unit			units::FPU;
    units::unit			units::BRU;

    namespace caches
    {
        cache   L1(params::L1::nsets, params::L1::nways, params::L1::linesize);

        cache::cache(uint32_t nsets, uint32_t nways, uint32_t linesize) : _sets(nsets)
        {
            _nsets = nsets;
            _nways = nways;
            _linesize = linesize;

            entry empty; empty.valid = false; empty.touched = 0;
            set   init(nways); for (uint32_t i=0; i<nways; i++) init[i] = empty;
            for (uint32_t i=0; i<nsets; i++) _sets[i] = init;
        }

        void cache::clear()
        {
            for (uint32_t setix=0; setix<nsets(); setix++)
                for (uint32_t wayix=0; wayix<nways(); wayix++)
                {
                    sets()[setix][wayix].valid = false;
                    sets()[setix][wayix].touched = 0;
                    sets()[setix][wayix].addr = 0;
                }
        }

        uint32_t cache::linesize() const
        {
            return _linesize;
        }

        uint32_t cache::nsets() const
        {
            return _nsets;
        }

        uint32_t cache::nways() const
        {
            return _nways;
        }

        uint32_t cache::capacity() const
        {
            return _nsets * _nways * _linesize;
        }

        array &cache::sets()
        {
            return _sets;
        }

        bool cache::hit(uint32_t addr)
        {
            counters::L1::accesses++;
            uint32_t lineaddr = addr / linesize();
            uint32_t setix = lineaddr % nsets();
            uint32_t wayix;
            for (wayix = 0; wayix < nways(); wayix++)
            {
                if (sets()[setix][wayix].valid && (sets()[setix][wayix].addr == lineaddr)) break;
            }
            if      (wayix < nways())
            {
                // L1 cache hit
                counters::L1::hits++;
                sets()[setix][wayix].touched = counters::cycles;
                return true;
            }
            else
            {
                // L1 cache miss
                counters::L1::misses++;
                // find the LRU entry
                uint64_t lasttouch = counters::cycles;
                uint32_t lru = nways();
                for (wayix = 0; wayix < nways(); wayix++)
                {
                    if (!sets()[setix][wayix].valid)
                    {
                        // invalid entry, can use this one as the lru
                        lru = wayix;
                        break;
                    }
                    if (sets()[setix][wayix].touched <= lasttouch)
                    {
                        // older than current candidate - update
                        lru = wayix;
                        lasttouch = sets()[setix][wayix].touched;
                    }
                }
                assert(lru < nways());
                sets()[setix][lru].valid = true;
                sets()[setix][lru].addr = lineaddr;
                sets()[setix][lru].touched = counters::cycles;
                return false;
            }
        }
    };

    flags_t     flags;                          // flags

    uint32_t    CIA;                            // current instruction address
    uint32_t    NIA;                            // next instruction address

    uint64_t    counters::instructions = 0;     // instruction counter
    uint64_t    counters::operations = 0;       // operation counter
    uint64_t    counters::cycles = 0;           // cycle counter
    uint64_t    counters::lastissued = 0;       // last issue cycle
    uint64_t    counters::L1::hits = 0;         // L1 hits
    uint64_t    counters::L1::misses = 0;       // L1 misses
    uint64_t    counters::L1::accesses = 0;     // L1 accesses

    void zeromem()
    {
        for (uint32_t i=0; i<params::MEM::N; i++) MEM[i] = 0;
    }

    void zeroctrs()
    {
	counters::instructions = 0;
	counters::operations = 0;
	counters::cycles = 0;
	counters::lastissued = 0;
        counters::L1::accesses = 0;
        counters::L1::hits = 0;
        counters::L1::misses = 0;
	for (uint32_t i=0; i<params::GPR::N; i++) GPR[i].ready() = 0;
	for (uint32_t i=0; i<params::FPR::N; i++) FPR[i].ready() = 0;
	units::FXU.ready() = 0;
	units::FPU.ready() = 0;
	units::LDU.ready() = 0;
	units::STU.ready() = 0;
	units::BRU.ready() = 0;
    }

    namespace operations
    {
	bool process(operation* op) { return op->process(); } 
    };

    namespace instructions
    {
	bool process(instruction* inst) { return inst->process(); }
    };
};
