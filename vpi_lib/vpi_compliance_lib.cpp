/**
 * TODO:License
 */

#include <unistd.h>
#include <stdlib.h>
#include <bitset>
#include <string>

extern "C" {
    #include "vpi_utils.h"
}

#include "vpi_compliance_lib.hpp"

/*****************************************************************************
 * Reset agent functions
 ****************************************************************************/

void reset_agent_assert()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_ASSERT);
    vpi_full_handshake();
}


void reset_agent_deassert()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_DEASSERT);
    vpi_full_handshake();
}


void reset_agent_polarity_set(int polarity)
{
    char pol_str[2];
    sprintf(pol_str, "%d", polarity);
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_POLARITY_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, pol_str);
    vpi_full_handshake();
}


int reset_agent_polarity_get()
{
    char pol_str[VPI_DBUF_SIZE];
    memset(pol_str, 0, sizeof(pol_str));
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_POLARITY_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, pol_str);
    vpi_end_handshake();
    return atoi(&(pol_str[0]));
}


/*****************************************************************************
 * Clock generator agent functions
 ****************************************************************************/

int clock_agent_start()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_START);
    vpi_full_handshake();
}


int clock_agent_stop()
{
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char*)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_STOP);
    vpi_full_handshake();
}


int clock_agent_set_period(std::chrono::nanoseconds clockPeriod)
{
    char vpi_data_in[VPI_DBUF_SIZE];
    unsigned long long timeVal = clockPeriod.count() * 1000000;
    sprintf(vpi_data_in, "%llu", timeVal);
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_PERIOD_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, vpi_data_in);
    vpi_full_handshake();
}


std::chrono::nanoseconds clock_agent_get_period()
{
    char vpi_data_out[VPI_DBUF_SIZE];
    std::chrono::nanoseconds timeVal;
    unsigned long long readTime;

    memset(vpi_data_out, 0, sizeof(vpi_data_out));
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_PERIOD_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    readTime = std::strtoll(vpi_data_out, nullptr, 2) / 1000000;
    timeVal = std::chrono::nanoseconds(readTime);
    vpi_end_handshake();

    return timeVal;
}


int clock_agent_set_jitter(std::chrono::nanoseconds clockPeriod)
{
    char vpi_data_in[VPI_DBUF_SIZE];
    unsigned long long timeVal = clockPeriod.count() * 1000000;
    sprintf(vpi_data_in, "%llu", timeVal);
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_JITTER_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, vpi_data_in);
    vpi_full_handshake();
}


std::chrono::nanoseconds clock_agent_get_jitter()
{
    char vpi_data_out[VPI_DBUF_SIZE];
    std::chrono::nanoseconds timeVal;
    unsigned long long readTime;

    memset(vpi_data_out, 0, sizeof(vpi_data_out));
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_JITTER_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    readTime = std::strtoll(vpi_data_out, nullptr, 2) / 1000000;
    timeVal = std::chrono::nanoseconds(readTime);
    vpi_end_handshake();

    return timeVal;
}


int clock_agent_duty_set(int duty)
{
    std::bitset<VPI_DBUF_SIZE> binVal(duty);
    char duty_str[VPI_DBUF_SIZE];
    std::string binValStr = binVal.to_string();

    for (int i = 0; i < VPI_DBUF_SIZE; i++)
        duty_str[i] = binValStr[i];

    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_RES_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_RST_AGNT_CMD_POLARITY_SET);
    vpiDriveStrValue(VPI_SIGNAL_DATA_IN, duty_str);
    vpi_full_handshake();
}


int clock_agent_duty_get()
{
    char vpi_data_out[VPI_DBUF_SIZE];
    int duty;

    memset(vpi_data_out, 0, sizeof(vpi_data_out));
    vpiDriveStrValue(VPI_SIGNAL_DEST, (char *)VPI_DEST_CLK_GEN_AGENT);
    vpiDriveStrValue(VPI_SIGNAL_CMD, (char*)VPI_CLK_AGNT_CMD_DUTY_GET);
    vpi_begin_handshake();
    vpiReadStrValue(VPI_SIGNAL_DATA_OUT, vpi_data_out);
    duty = std::stoi(vpi_data_out, nullptr, 2);
    vpi_end_handshake();

    return duty;
}