#ifndef TEST_SEQUENCE_H
#define TEST_SEQUENCE_H
/******************************************************************************
 *
 * ISO16845 Compliance tests
 * Copyright (C) 2021-present Ondrej Ille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this SW component and associated documentation files (the "Component"),
 * to use, copy, modify, merge, publish, distribute the Component for
 * educational, research, evaluation, self-interest purposes. Using the
 * Component for commercial purposes is forbidden unless previously agreed with
 * Copyright holder.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Component.
 *
 * THE COMPONENT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHTHOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE COMPONENT OR THE USE OR OTHER DEALINGS
 * IN THE COMPONENT.
 *
 * @author Ondrej Ille, <ondrej.ille@gmail.com>
 * @date 27.3.2020
 *
 *****************************************************************************/

#include <iostream>
#include <string>
#include <chrono>
#include <vector>

#include <can_lib.h>
#include <pli_lib.h>

#include "test.h"
#include "MonitorItem.h"
#include "DriverItem.h"

/**
 * @namespace test
 * @class TestSequence
 * @brief Test sequence for simulator.
 *
 * Test sequence contains sequence for CAN Agent driver and for CAN Agent
 * monitor. Driver sequence will be driven by CAN agent to "can_rx" of DUT
 * and Monitor sequence will be checked by CAN agent on "can_tx" of DUT.
 */
class test::TestSequence
{
    public:
        TestSequence(std::chrono::nanoseconds clock_period);
        TestSequence(std::chrono::nanoseconds clock_period, can::BitFrame& frame,
                     SequenceType sequence_type);
        TestSequence(std::chrono::nanoseconds clock_period, can::BitFrame& driver_frame,
                     can::BitFrame& monitor_frame);

        /**
         * @brief Gets pointer to n-th monitor item.
         * @param index Index of monitor item in monitor sequence.
         *
         * @return Pointer to monitor item at position "index". NULL if index is
         *         larger than number of monitor items in sequence.
         */
        MonitorItem* GetMonitorItem(int index);

        /**
         * @brief Gets pointer to n-th driver item.
         * @param index Index of driver item in driver sequence.
         *
         * @return Pointer to driver item at position "index". NULL if index is
         *         larger than number of driver items in sequence.
         */
        DriverItem *GetDriverItem(int index);

        /**
         * @brief Appends monitor item to driver sequence
         * @param driver_item Item to be apended.
         */
        void AppendDriverItem(DriverItem driver_item);

        /**
         * @brief Prints items in driver sequence.
         */
        void PrintDrivenValues();

        /**
         * @brief Prints items in monitor sequence.
         */
        void PrintMonitoredValues();

        /**
         * @brief Copies items from driver sequence to CAN Agent driver FIFO in
         *        simulator.
         * @note It is good to flush the FIFO before!
         * @note If overflow of FIFO occurs, this function ignores it!
         */
        void PushDriverValuesToSimulator();

        /**
         * @brief Copies items from monitor sequence to CAN Agent driver FIFO in
         *        simulator.
         * @note It is good to flush the FIFO before!
         * @note If overflow of FIFO occurs, this function ignores it!
         */
        void PushMonitorValuesToSimulator();

    private:

        /**
         * Vectors of driver/monitor items.
         */
        std::vector<DriverItem> driven_values;
        std::vector<MonitorItem> monitored_values;

        /**
         * Clock period configured in simulator for DUT operation. This
         * information is used to calculate proper duration of each monitor/driver
         * item.
         */
        std::chrono::nanoseconds clock_period;

        /**
         * @brief Appends CAN frame to driver sequence.
         *
         * CAN frame is converted to sequence of driver items for CAN Agent
         * driver and appended to "drivenValues". Each bit on CAN bus is converted
         * to single driver item.
         *
         * @param bit_frame Reference to CAN frame to be converted. Not modified.
         */
        void AppendDriverFrame(can::BitFrame& bit_frame);

        /**
         * @brief Appends CAN frame to monitor sequence.
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
         * @param bit_frame Reference to CAN frame to be converted. Not modified.
         */
        void AppendMonitorFrame(can::BitFrame& bit_frame);

        /**
         * @brief Appends single CAN bit to driver items sequence.
         *
         * CAN bit is converted to single driver item sequence.
         *
         * @param bit CAN bit to append. Not modified
         */
        void AppendDriverBit(can::Bit *bit);

        /**
         * @brief Appends single CAN bit to monitor items sequence.
         *
         * CAN bit is converted to single monitor item sequence.
         *
         * @param bit CAN bit to append. Not modified
         */
        void AppendMonitorBit(can::Bit *bit);

        /**
         * @brief Appends bit during which bit rate shift occurs (BRS, CRC delimiter).
         *
         * @param bit CAN bit to append. Not modified.
         */
        void appendMonitorBitWithShift(can::Bit *bit);

        /**
         * @brief Appends bit during which bit rate shift does not occurs.
         * @param bit CAN bit to append. Not modified.
         */
        void appendMonitorNotShift(can::Bit *bit);

        /**
         * @brief Pushes an item into driver FIFO.
         * @param duration Duration of item for which it shall be driven.
         * @param bit_value Bit value to be driven.
         * @param message Message to be printed when driving starts.
         */
        void pushDriverValue(std::chrono::nanoseconds duration,
                             can::BitValue bit_value,
                             std::string message);

        /**
         * @brief Pushes an item into monitor FIFO.
         * @param duration Duration of item for which it shall be monitored.
         * @param sample_rate How often monitor shall check the value during monitoring.
         * @param bit_value Bit value to be monitored.
         * @param message Message to be printed when monitoring starts.
         */
        void pushMonitorValue(std::chrono::nanoseconds duration,
                              std::chrono::nanoseconds sample_rate,
                              can::BitValue bit_value,
                              std::string message);
};

#endif