/*
 * TODO: License
 */

#include <iostream>
#include <cstdint>
#include <list>

#include "can.h"
#include "CycleBitValue.h"

#ifndef TIME_QUANTA
#define TIME_QUANTA

class can::TimeQuanta
{
    public:
        /**
         *
         */
        TimeQuanta();

        /**
         *
         */
        TimeQuanta(int brp, BitPhase bitPhase);

        /**
         *
         */
        TimeQuanta(int brp, BitPhase bitPhase, BitValue bitValue);

        /**
         * 
         */
        bool hasNonDefaultValues();

        /**
         *
         */
        void setAllDefaultValues();

        /**
         *
         */
        int getLengthCycles();

        /**
         *
         */
        void lengthen(int byCycles);

        /**
         *
         */
        void lengthen(int byCycles, BitValue bitValue);

        /**
         *
         */
        void shorten(int byCycles);

        /**
         *
         */
        bool forceCycleValue(int cycleIndex, BitValue bitValue);

        /**
         *
         */
        bool forceCycleValue(int cycleIndexFrom, int cycleIndexTo, BitValue bitValue);

        /**
         *
         */
        bool forceValue(BitValue bitValue);

        /**
         * 
         */
        BitPhase bitPhase;

    private:
        std::list<CycleBitValue> cycleBitValues_;
};

#endif