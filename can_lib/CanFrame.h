/*
 * TODO: License
 */

#include <cstdint>
#include "can.h"

using namespace can;

#ifndef CAN_FRAME
#define CAN_FRAME

class CanFrame {

    public:
        CanFrame();
        CanFrame(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                 RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                 ErrorStateIndicator isEsi, uint8_t dlc, int identifier,
                 uint8_t *data);
        CanFrame(FlexibleDataRate isFdf, ExtendedIdentifier isIde,
                 RemoteTransmissionRequest isRtr, BitRateShift isBrs,
                 ErrorStateIndicator isEsi, int dataLength,
                 int identifier, uint8_t *data);
        void Copy(CanFrame canFrame);

        // Getters and setters are declared on this data structure to avoid
        // invalid frame combinations (e.g. RTR frame with FDF flag)

        FlexibleDataRate getFdf();
        ExtendedIdentifier getIde();
        RemoteTransmissionRequest getRtr();
        BitRateShift getBrs();
        ErrorStateIndicator getEsi();
        uint8_t getDlc();
        int getDataLenght();
        int getIdentifier();
        uint8_t* getData();
        uint8_t getData(int index);

        void setFdf(FlexibleDataRate isFdf);
        void setIde(ExtendedIdentifier isIde);
        void setRtr(RemoteTransmissionRequest isRtr);
        void setBrs(BitRateShift isBrs);
        void setDlc(uint8_t dlc);
        void setEsi(ErrorStateIndicator isEsi);
        bool setDataLenght(int dataLenght);
        void setIdentifer(int identifier);
        void copyData(uint8_t *data, int dataLen);

        void print();

    protected:
        FlexibleDataRate isFdf_;
        ExtendedIdentifier isIde_;
        RemoteTransmissionRequest isRtr_;
        BitRateShift isBrs_;
        ErrorStateIndicator isEsi_;

        // Data length code
        uint8_t dlc_;

        // Data length (always should reflect data length)
        int dataLenght_;

        // Frame identfier
        int identifier_;

        // Data payload
        uint8_t data_[64];

        // Supported DLC / Datalength combinations
        const int dlcToDataLenghtTable_ [16][2] =
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

        int convertDlcToDataLenght(uint8_t dlc);
        uint8_t convertDataLenghtToDlc(int dataLenght);
        bool isValidDataLength(int dataLenght);
};

#endif