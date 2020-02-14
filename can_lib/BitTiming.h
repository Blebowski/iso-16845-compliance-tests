/*
 * TODO: License
 */

#ifndef BIT_TIMING
#define BIT_TIMING

class BitTiming
{
    public:
        unsigned int propNbt;
        unsigned int ph1Nbt;
        unsigned int ph2Nbt;
        unsigned int tqNbt;
        unsigned int sjwNbt;

        unsigned int propDbt;
        unsigned int ph1Dbt;
        unsigned int ph2Dbt;
        unsigned int tqDbt;
        unsigned int sjwDbt;

        BitTiming(unsigned int propNbt, unsigned int ph1Nbt, unsigned int ph2Nbt,
                  unsigned int tqNbt, unsigned int sjwNbt, unsigned int propDbt,
                  unsigned int ph1Dbt, unsigned int ph2Dbt, unsigned int tqDbt,
                  unsigned int sjwDbt);
};

#endif