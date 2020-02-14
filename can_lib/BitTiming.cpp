/*
 * TODO: License
 */

#include <assert.h>

#include "BitTiming.h"


BitTiming::BitTiming(unsigned int propNbt, unsigned int ph1Nbt, unsigned int ph2Nbt,
                     unsigned int tqNbt, unsigned int sjwNbt, unsigned int propDbt,
                     unsigned int ph1Dbt, unsigned int ph2Dbt, unsigned int tqDbt,
                     unsigned int sjwDbt)
{
    this->propNbt = propNbt;
    this->ph1Nbt = ph1Nbt;
    this->ph2Nbt = ph2Nbt;
    this->sjwNbt = sjwNbt;
    this->tqNbt = tqNbt;

    this->propDbt = propDbt;
    this->ph1Dbt = ph1Dbt;
    this->ph2Dbt = ph2Dbt;
    this->sjwDbt = sjwDbt;
    this->tqDbt = tqDbt;

    // SJW can't be larger than any of TSEG1 or TSEG2. Don't account any IPT
    // as this model is ideal implementation
    assert(sjwNbt <= propNbt + ph1Nbt + 1);
    assert(sjwNbt <= ph2Nbt);

    assert(sjwDbt <= propDbt + ph1Dbt + 1);
    assert(sjwDbt <= ph2Dbt);
}