/**
 * TODO: License
 */

#include "test_lib.h"
#include "../can_lib/can.h"

#include "TestSequence.h"
#include "../can_lib/BitFrame.h"

#include "../vpi_lib/vpiComplianceLib.hpp"

test_lib::TestSequence::TestSequence(std::chrono::nanoseconds clockPeriod)
{
    this->clockPeriod = clockPeriod;
};


test_lib::TestSequence::TestSequence(std::chrono::nanoseconds clockPeriod,
                                     can::BitFrame& frame,
                                     SequenceType sequenceType)
{
    this->clockPeriod = clockPeriod;

    if (sequenceType == DRIVER_SEQUENCE) {
        drivenValues.clear();
        appendDriverFrame(frame);
    } else {
        monitoredValues.clear();
        appendMonitorFrame(frame);
    }
};


test_lib::TestSequence::TestSequence(std::chrono::nanoseconds clockPeriod,
                                     can::BitFrame& driveFrame,
                                     can::BitFrame& monitorFrame)
{
    this->clockPeriod = clockPeriod;
    monitoredValues.clear();
    drivenValues.clear();

    appendMonitorFrame(monitorFrame);
    appendDriverFrame(driveFrame);
}


void test_lib::TestSequence::appendDriverFrame(can::BitFrame& driveFrame)
{
    int bitCount = driveFrame.getBitCount();
    can::Bit *bit;

    for (int i = 0; i < bitCount; i++)
    {
        bit = driveFrame.getBit(i);
        appendBit<DriverItem>(drivenValues, bit);
    }
}


void test_lib::TestSequence::appendMonitorFrame(can::BitFrame& monitorFrame)
{
    int bitCount = monitorFrame.getBitCount();
    can::Bit *bit;

    for (int i = 0; i < bitCount; i++)
    {
        bit = monitorFrame.getBit(i);
        appendBit<MonitorItem>(monitoredValues, bit);
    }
}


template <class Item>
void test_lib::TestSequence::appendBit(std::vector<Item>& vector, can::Bit* bit)
{
    int timeQuantas = bit->getLenTimeQuanta();
    can::BitValue bitValue = bit->getBitValue();
    can::BitValue lastValue = bitValue;
    can::BitValue currentValue;
    can::TimeQuanta *timeQuanta;
    can::CycleBitValue* cycleBitValue;
    std::chrono::nanoseconds duration (0);
    int cycles;

    for (int i = 0; i < timeQuantas; i++)
    {
        timeQuanta = bit->getTimeQuanta(i);
        cycles = timeQuanta->getLengthCycles();

        for (int j = 0; j < cycles; j++)
        {
            cycleBitValue = timeQuanta->getCycleBitValue(j);

            if (cycleBitValue->hasDefaultValue == true)
                currentValue = bitValue;
            else
                currentValue = cycleBitValue->bitValue;

            // Note: This ignores non-default values which are equal to
            //       its default value (as expected) and merges them into
            //       single monitored / driven item!

            // If we did not detect bit value change -> it still belongs to the
            // same segment -> legnthen it
            if (currentValue == lastValue)
                duration += clockPeriod;

            // We detected value change or are at the end of bit, add item.
            if (currentValue != lastValue ||
                ((i == timeQuantas - 1) && (j == cycles - 1))) {
                // TODO: Push with message on first Item of bit
                // TODO: Push with message on each next item signalling glitch!
                
                if (duration.count() > 0)
                    pushValue(vector, duration, currentValue);
                duration = clockPeriod;
                lastValue = currentValue;
            }
        }
    }
}


template <class Item>
void test_lib::TestSequence::pushValue(std::vector<Item>& vector,
                                       std::chrono::nanoseconds duration,
                                       can::BitValue bitValue)
{
    // TODO: This conversion should ideally be separated!
    StdLogic logicVal;
    if (bitValue == can::DOMINANT)
        logicVal = LOGIC_0;
    else
        logicVal = LOGIC_1;

    vector.push_back(Item(duration, logicVal));
}


void test_lib::TestSequence::printDrivenValues()
{
    for (auto drivenValue : drivenValues)
        drivenValue.print();
    std::cout << std::endl;
}


void test_lib::TestSequence::printMonitoredValues()
{
    for (auto monitoredValue : monitoredValues)
        monitoredValue.print();
    std::cout << std::endl;
}


void test_lib::TestSequence::pushDriverValuesToSimulator()
{
    char val;
    for (auto drivenValue : drivenValues)
    {
        if (drivenValue.hasMessage())
            canAgentDriverPushItem(drivenValue.value, drivenValue.duration,
                                    drivenValue.message);
        else
            canAgentDriverPushItem(drivenValue.value, drivenValue.duration);
    }
}