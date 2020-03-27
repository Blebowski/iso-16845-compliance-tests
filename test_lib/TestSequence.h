/****************************************************************************** 
 * 
 * @copyright Copyright (C) Ondrej Ille - All Rights Reserved
 * 
 * Copying, publishing, distributing of this file is stricly prohibited unless
 * previously aggreed with author of this text.
 * 
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 * 
 *****************************************************************************/

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

/**
 * @namespace test_lib
 * @class TestSequence
 * @brief Test sequence for simulator.
 * 
 * Test sequence contains sequence for CAN Agent driver and for CAN Agent
 * monitor. Driver sequence will be driven by CAN agent to "can_rx" of DUT
 * and Monitor sequence will be checked by CAN agent on "can_tx" of DUT.
 */
class test_lib::TestSequence
{
    public:
        TestSequence(std::chrono::nanoseconds clockPeriod);
        TestSequence(std::chrono::nanoseconds clockPeriod, can::BitFrame& frame,
                     SequenceType sequenceType);
        TestSequence(std::chrono::nanoseconds clockPeriod, can::BitFrame& driveFrame,
                     can::BitFrame& monitorFrame);

        /**
         * @brief Get pointer to n-th monitor item.
         * @param index Index of monitor item in monitor sequence.
         * 
         * @return Pointer to monitor item at position "index". NULL if index is
         *         larger than number of monitor items in sequence.
         */
        MonitorItem* getMonitorItem(int index);

        /**
         * @brief Get pointer to n-th driver item.
         * @param index Index of driver item in driver sequence.
         * 
         * @return Pointer to driver item at position "index". NULL if index is
         *         larger than number of driver items in sequence.
         */
        DriverItem *getDriverItem(int index);

        /**
         * @brief Append monitor item to monitor sequence
         * @param monitorItem Item to be apended.
         */
        void appendMonitorItem(MonitorItem monitorItem);

        /**
         * @brief Append monitor item to driver sequence
         * @param driverItem Item to be apended.
         */
        void appendDriverItem(DriverItem driverItem);

        /**
         * @brief Print items in driver sequence.
         */
        void printDrivenValues();
        
        /**
         * @brief Print items in monitor sequence.
         */
        void printMonitoredValues();

        /**
         * @brief Copy items from driver sequence to CAN Agent driver FIFO in
         *        simulator.
         * @note It is good to flush the FIFO before!
         * @note If overflow of FIFO occurs, this function ignores it!
         */
        void pushDriverValuesToSimulator();

        /**
         * @brief Copy items from monitor sequence to CAN Agent driver FIFO in
         *        simulator.
         * @note It is good to flush the FIFO before!
         * @note If overflow of FIFO occurs, this function ignores it!
         */
        void pushMonitorValuesToSimulator();

    private:

        /**
         * Vectors of driver/monitor items.
         */
        std::vector<DriverItem> drivenValues;
        std::vector<MonitorItem> monitoredValues;

        /**
         * Clock period configured in simulator for DUT operation. This
         * information is used to calculate proper duration of each monitor/driver
         * item.
         */
        std::chrono::nanoseconds clockPeriod;

        /**
         * @brief Append CAN frame to driver sequence.
         * 
         * CAN frame is converted to sequence of driver items for CAN Agent
         * driver and appended to "drivenValues". Each bit on CAN bus is converted
         * to single driver item.
         * 
         * @param bitFrame Reference to CAN frame to be converted. Not modified.
         */
        void appendDriverFrame(can::BitFrame& bitFrame);

        /**
         * @brief Append CAN frame to monitor sequence.
         * 
         * CAN frame is converted to sequence of monitor items for CAN Agent
         * monitor and appended to "monitorValues".
         * 
         * Bits BRS and CRC delimiters are converted to two monitor items. All
         * other bits are converted to single monitor item. Duration of monitor
         * item is equal to duration of the bit on CAN bus. Sample rate of each
         * item is equal to Baud rate prescaler used during that bit.
         * 
         * @warning BRS and CRC delimiter encoding causes that CAN agent monitor
         *          does not perform check exactly in sample point of these bits!
         * 
         * @param bitFrame Reference to CAN frame to be converted. Not modified.
         */
        void appendMonitorFrame(can::BitFrame& bitFrame);

        /**
         * @brief Append single CAN bit to driver items sequence.
         * 
         * CAN bit is converted to single driver item sequence.
         * 
         * @param bit CAN bit to append. Not modified
         */
        void appendDriverBit(can::Bit *bit);

        /**
         * @brief Append single CAN bit to monitor items sequence.
         * 
         * CAN bit is converted to single monitor item sequence.
         * 
         * @param bit CAN bit to append. Not modified
         */
        void appendMonitorBit(can::Bit *bit);

        // TODO
        void appendMonitorBitWithShift(can::Bit *bit);
        void appendMonitorNotShift(can::Bit *bit);

        //TODO
        void pushDriverValue(std::chrono::nanoseconds duration,
                             can::BitValue bitValue);
        void pushMonitorValue(std::chrono::nanoseconds duration,
                              std::chrono::nanoseconds sampleRate,
                              can::BitValue bitValue);
};

#endif