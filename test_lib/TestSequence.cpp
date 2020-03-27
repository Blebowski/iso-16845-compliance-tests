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
        appendDriverBit(drivenValues, bit);
    }
}


void test_lib::TestSequence::appendMonitorFrame(can::BitFrame& monitorFrame)
{
    int bitCount = monitorFrame.getBitCount();
    can::Bit *bit;

    for (int i = 0; i < bitCount; i++)
    {
        bit = monitorFrame.getBit(i);
        appendMonitorBit(monitoredValues, bit);
    }
}


void test_lib::TestSequence::appendDriverBit(std::vector<DriverItem>& vector, can::Bit* bit)
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
                    pushDriverValue(vector, duration, currentValue);
                duration = clockPeriod;
                lastValue = currentValue;
            }
        }
    }
}

void test_lib::TestSequence::appendMonitorBitWithShift(
                std::vector<MonitorItem> &vector, can::Bit *bit)
{
    std::chrono::nanoseconds tseg1Duration (0);
    std::chrono::nanoseconds tseg2Duration (0);
    int tseg1Len = bit->getPhaseLenTimeQuanta(can::SYNC_PHASE);
    tseg1Len += bit->getPhaseLenTimeQuanta(can::PROP_PHASE);
    tseg1Len += bit->getPhaseLenTimeQuanta(can::PH1_PHASE);
    int tseg2Len = bit->getPhaseLenTimeQuanta(can::PH2_PHASE);

    for (int i = 0; i < tseg1Len; i++)
        for (int j = 0; j < bit->getTimeQuanta(i)->getLengthCycles(); j++)
            tseg1Duration += clockPeriod;

    for (int i = 0; i < tseg2Len; i++)
        for (int j = 0; j < bit->getTimeQuanta(i)->getLengthCycles(); j++)
            tseg2Duration += clockPeriod;

    int brp = bit->getTimeQuanta(can::SYNC_PHASE, 0)->getLengthCycles();
    int brpFd = bit->getTimeQuanta(can::PH2_PHASE, 0)->getLengthCycles();
    std::chrono::nanoseconds sampleRateNominal = brp * clockPeriod;
    std::chrono::nanoseconds sampleRateData = brpFd * clockPeriod;

    pushMonitorValue(vector, tseg1Duration, sampleRateNominal, bit->getBitValue());
    pushMonitorValue(vector, tseg2Duration, sampleRateData, bit->getBitValue());
}


void test_lib::TestSequence::appendMonitorNotShift(
                std::vector<MonitorItem> &vector, can::Bit *bit)
{
    std::chrono::nanoseconds duration (0);

    for (int i = 0; i < bit->getLenTimeQuanta(); i++)
        for (int j = 0; j < bit->getTimeQuanta(i)->getLengthCycles(); j++)
            duration += clockPeriod;

    // Assume first Time quanta length is the same as rest (which is reasonable)!
    int brp = bit->getTimeQuanta(0)->getLengthCycles();
    std::chrono::nanoseconds sampleRate = brp * clockPeriod;

    pushMonitorValue(vector, duration, sampleRate, bit->getBitValue());
}


void test_lib::TestSequence::appendMonitorBit(std::vector<MonitorItem>& vector, can::Bit* bit)
{
    can::BitValue currentValue;
    can::TimeQuanta *timeQuanta;
    can::CycleBitValue* cycleBitValue;
    std::chrono::nanoseconds duration (0);
    int cycles;

    if (bit->getBitType() == can::BIT_TYPE_BRS ||
        bit->getBitType() == can::BIT_TYPE_CRC_DELIMITER)
        appendMonitorBitWithShift(vector, bit);
    else
        appendMonitorNotShift(vector, bit);
}


void test_lib::TestSequence::pushDriverValue(std::vector<DriverItem> &vector,
                                              std::chrono::nanoseconds duration,
                                              can::BitValue bitValue)
{
    // TODO: This conversion should ideally be separated!
    StdLogic logicVal;
    if (bitValue == can::DOMINANT)
        logicVal = LOGIC_0;
    else
        logicVal = LOGIC_1;

    vector.push_back(DriverItem(duration, logicVal));
}


void test_lib::TestSequence::pushMonitorValue(std::vector<MonitorItem> &vector,
                                              std::chrono::nanoseconds duration,
                                              std::chrono::nanoseconds sampleRate,
                                              can::BitValue bitValue)
{
    // TODO: This conversion should ideally be separated!
    StdLogic logicVal;
    if (bitValue == can::DOMINANT)
        logicVal = LOGIC_0;
    else
        logicVal = LOGIC_1;

    vector.push_back(MonitorItem(duration, logicVal, sampleRate));
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
    for (auto drivenValue : drivenValues)
    {
        if (drivenValue.hasMessage())
            canAgentDriverPushItem(drivenValue.value, drivenValue.duration,
                                    drivenValue.message);
        else
            canAgentDriverPushItem(drivenValue.value, drivenValue.duration);
    }
}


void test_lib::TestSequence::pushMonitorValuesToSimulator()
{
    for (auto monitorValue : monitoredValues)
    {
        if (monitorValue.hasMessage())
            canAgentMonitorPushItem(monitorValue.value, monitorValue.duration,
                                    monitorValue.sampleRate, monitorValue.message);
        else
            canAgentMonitorPushItem(monitorValue.value, monitorValue.duration,
                                    monitorValue.sampleRate);
    }
}