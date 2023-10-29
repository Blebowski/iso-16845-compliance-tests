#ifndef FRAME_H
#define FRAME_H
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

#include <cstdint>

#include "can.h"
#include "FrameFlags.h"


/**
 * @class Frame
 * @namespace can
 *
 * Represents CAN frame and its metadata (DLC, ID, Data, etc...). Used for transmission,
 * reception by DUT. Does not hold cycle-accurate representation of frame, BitFrame class
 * is intended for that.
 */
class can::Frame {

    public:

        /*
         * A constructor which does not specify all attributes, leaves these arguments enabled
         * for randomization. Attributes will be randomized upon call of 'randomize'.
         */

        /**
         * Randomizes everything
         */
        Frame();

        /**
         * Does not randomize
         * @param frm_flags Frame flags of frame
         * @param dlc Data length code
         * @param identifier CAN Identifier
         * @param data Data payload
         */
        Frame(FrameFlags frm_flags, uint8_t dlc, int ident, uint8_t *data);

        /**
         * Randomizes data.
         * @param frm_flags Frame flags of frame
         * @param dlc Data length code
         * @param identifier CAN Identifier
         */
        Frame(FrameFlags frm_flags, uint8_t dlc, int ident);

        /**
         * Randomizes data and identifier
         * @param frm_flags Frame flags of frame
         * @param dlc Data length code
         */
        Frame(FrameFlags frm_flags, uint8_t dlc);

        /**
         * Randomizes data, identifier and DLC.
         * @param frm_flags Frame flags of frame
         */
        Frame(FrameFlags frm_flags);

        /**
         * Randomizes identifier
         * @param frm_flags Frame flags of frame
         * @param dlc Data length code
         * @param data Data payload
         */
        Frame(FrameFlags frm_flags, uint8_t dlc, uint8_t *data);

        /**
         * Getters.
         */
        inline FrameFlags frame_flags() const {
            return frm_flags_;
        };

        inline uint8_t dlc() const {
            return dlc_;
        };

        inline int data_length() const {
            return data_len_;
        };

        inline int identifier() const {
            return ident_;
        };

        inline uint8_t* data() {
            return data_;
        };

        inline uint8_t data(int index) const {
            return data_[index];
        };

        /**
         * Randomize attribute which are enabled for randomization (see constructor).
         */
        void Randomize();

        /**
         * Prints frame to stdout.
         */
        void Print();

        /**
         * Compare overloads
         */
        bool operator==(const Frame rhs);
        bool operator!=(const Frame rhs);

        FrameFlags frm_flags_;

    protected:

        // Data length code
        uint8_t dlc_ = 0;

        // Data length (always should reflect data length)
        int data_len_ = 0;

        // Frame identfier
        int ident_ = 0;

        // Data payload
        uint8_t data_[64] = {0x0};

        // Randomization attributes
        bool randomize_dlc_ = false;
        bool randomize_ident_ = false;
        bool randomize_data_ = false;

        // Supported DLC / Datalength combinations
        const int dlc_to_data_length_table_ [16][2] =
        {
            {0b0000, 0},
            {0b0001, 1},
            {0b0010, 2},
            {0b0011, 3},
            {0b0100, 4},
            {0b0101, 5},
            {0b0110, 6},
            {0b0111, 7},
            {0b1000, 8},
            {0b1001, 12},
            {0b1010, 16},
            {0b1011, 20},
            {0b1100, 24},
            {0b1101, 32},
            {0b1110, 48},
            {0b1111, 64}
        };

        /**
         * Sets DLC and data lenght from DLC. Aborts on invalid DLC.
         * @param dlc DLC to be set
         */
        void set_dlc(uint8_t dlc);

        /**
         * Sets data lenght and DLC from data length. Aborts on invalid data length.
         * @param data_length data length to be set
         */
        void set_data_length(int data_len);

        /**
         * Sets identifier, aborts if identifier is out of range.
         * @param identifier Identifier in range  0 - 2^11-1 (Base) , 0 - 2^29-1(Extended)
         * Aborts on invalid identifier.
         */
        void set_identifier(int ident);

        /**
         * Copies data into frame data.
         * @param data Pointer to data being copied
         * @param len Number of bytes to be copied
         */
        void CopyData(uint8_t *data, int len);

        /**
         * @param dlc DLC to be converted
         * @returns dlc converted to according data length in bytes
         */
        int ConvDlcToDataLen(uint8_t dlc);

        /**
         * @param data_len Data length to be converted
         * @returns Data length converted to DLC.
         */
        uint8_t ConvDataLenToDlc(int data_len);

        /**
         * Checks whether data length is valid data payload length for CAN frame.
         * @param data_len data length to be checked.
         * @returns true if it is, false otherwise
         */
        bool IsValidDataLen(int data_len);
};

#endif