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

        /**
         * 
         */
        void pushDriverValuesToSimulator();

        /**
         * 
         */
        void pushMonitorValuesToSimulator();

    private:
        std::vector<DriverItem> drivenValues;
        std::vector<MonitorItem> monitoredValues;

        std::chrono::nanoseconds clockPeriod;

        void appendDriverFrame(can::BitFrame& bitFrame);
        void appendMonitorFrame(can::BitFrame& bitFrame);

        void appendDriverBit(std::vector<DriverItem> &vector, can::Bit *bit);
        void appendMonitorBit(std::vector<MonitorItem> &vector, can::Bit *bit);

        void appendMonitorBitWithShift(std::vector<MonitorItem> &vector, can::Bit *bit);
        void appendMonitorNotShift(std::vector<MonitorItem> &vector, can::Bit *bit);

        void pushDriverValue(std::vector<DriverItem> &vector,
                             std::chrono::nanoseconds duration,
                             can::BitValue bitValue);
        void pushMonitorValue(std::vector<MonitorItem> &vector,
                              std::chrono::nanoseconds duration,
                              std::chrono::nanoseconds sampleRate,
                              can::BitValue bitValue);
};

#endif