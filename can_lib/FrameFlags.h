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

#include "can.h"

#ifndef FRAME_FLAGS
#define FRAME_FLAGS


/**
 * @class FrameFlags
 * @namespace can
 * 
 * Represents flags of CAN frame (IDE, RTR, BRS, ESI, FDF(EDL))
 */
class can::FrameFlags
{
    public:

        /* CAN frame flags */
        FrameType is_fdf_;
        IdentifierType is_ide_;
        RtrFlag is_rtr_;
        BrsFlag is_brs_;
        EsiFlag is_esi_;

        /* Randomization attributes */
        bool randomize_fdf;
        bool randomize_ide;
        bool randomize_rtr;
        bool randomize_brs;
        bool randomize_esi;

        /* 
         * A constructor which does not specify all attributes, leaves these arguments enabled
         * for randomization. Attributes will be randomized upon call of 'randomize'.
         * 
         * All values are randomized in valid ranges. There can never be e.g. CAN FD frame with
         * RTR flag or CAN 2.0 frame with BRS flag.
         */

        /**
         * Everything is randomized.
         */
        FrameFlags();

        /**
         * Nothing is randomized
         */
        FrameFlags(FrameType is_fdf, IdentifierType is_ide,
                   RtrFlag is_rtr, BrsFlag is_brs,
                   EsiFlag is_esi);

        /**
         *  BRS and ESI are randomized (if frame is CAN FD frame)
         */
        FrameFlags(FrameType is_fdf, IdentifierType is_ide,
                   RtrFlag is_rtr);

        /**
         * Randomizes RTR flag, BRS and ESI
         */
        FrameFlags(FrameType is_fdf, IdentifierType is_ide);

        /**
         * Randomizes IDE, BRS, ESI
         */
        FrameFlags(FrameType is_fdf, RtrFlag is_rtr);

        /**
         * Randomizes IDE, BRS, ESI, RTR.
         */
        FrameFlags(FrameType is_fdf);

        /**
         * Randomizes FDF, BRS, ESI, RTR
         */
        FrameFlags(IdentifierType is_ide);

        /**
         * Randomizes ESI, BRS, RTR
         */
        FrameFlags(FrameType is_fdf, BrsFlag is_brs);

        /**
         * Randomizes RTR and IDE
         */
        FrameFlags(FrameType is_fdf, BrsFlag is_brs, EsiFlag is_esi);

        /**
         * Randomizes BRS, RTR, IDE
         */
        FrameFlags(FrameType is_fdf, EsiFlag is_esi);

        /**
         * Randomizes RTR, BRS
         */
        FrameFlags(FrameType is_fdf, IdentifierType is_ide, EsiFlag is_esi);

        /**
         * Randomizes frame flags.
         */
        void Randomize();

        /**
         * Enables all atributes/flags for randomization.
         */
        void RandomizeEnableAll();

        /**
         * Disables all atributes/flags for randomization.
         */
        void RandomizeDisableAll();

        friend bool operator==(const FrameFlags &lhs, const FrameFlags rhs);

    private:

        /**
         * Corrects frame flag combinations to valid values. E.g. CAN FD and RTR
         * flags can't be set at once (CAN FD frames have no RTR flags). Similarly
         * CAN 2.0 frames have no BRS flags or ESI Flags.
         */
        void CorrectFlags();
};

#endif