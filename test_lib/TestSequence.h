/*
 * TODO: License
 */

#include <iostream>
#include <string>
#include <chrono>
#include <vector>

#include "../can_lib/can.h"
#include "test_lib.h"

#include "MonitorItem.h"
#include "DriverItem.h"

#ifndef TEST_SEQUENCE
#define TEST_SEQUENCE

class test_lib::TestSequence
{
    public:
        TestSequence(std::chrono::nanoseconds clockPeriod);
        TestSequence(std::chrono::nanoseconds clockPeriod, can::BitFrame& frame,
                     SequenceType sequenceType);
        TestSequence(std::chrono::nanoseconds clockPeriod, can::BitFrame& driveFrame,
                     can::BitFrame& monitorFrame);

        /**
         * 
         */
        MonitorItem* getMonitorItem(int index);

        /**
         *
         */
        DriverItem *getDriverItem(int index);

        /**
         *
         */
        void appendMonitorItem(MonitorItem monitorItem);

        /**
         *
         */
        void appendDriverItem(DriverItem driverItem);

        /**
         *
         */
        void printDrivenValues();
        
        /**
         * 
         */
        void printMonitoredValues();

    private:
        std::vector<DriverItem> drivenValues;
        std::vector<MonitorItem> monitoredValues;

        std::chrono::nanoseconds clockPeriod;

        void appendDriverFrame(can::BitFrame& bitFrame);
        void appendMonitorFrame(can::BitFrame& bitFrame);

        template <class Item>
        void appendBit(std::vector<Item>& vector, can::Bit *bit);

        template <class Item>
        void pushValue(std::vector<Item> &vector,
                       std::chrono::nanoseconds duration,
                       can::BitValue bitValue);
};

#endif