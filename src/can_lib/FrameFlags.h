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

        /* 
         * A constructor call which does not specify all attributes, leaves these arguments enabled
         * for randomization. Attributes will be randomized upon call of 'Randomize'.
         * 
         * All values are randomized in valid ranges. There can never be e.g. CAN FD frame with
         * RTR flag or CAN 2.0 frame with BRS flag.
         * 
         * Example:
         *   FrameFlags(FramType:Can_2_0, RtrFlag::DataFrame)
         * will create CAN 2.0 Data frame, BRS, ESI will be kept default (do not exist in
         * CAN 2.0), and Identifier type will be allows for randomization.
         */

        /**
         * Everything is randomized.
         */
        FrameFlags();

        /**
         * Nothing is randomized.
         */
        FrameFlags(FrameType is_fdf, IdentifierType is_ide, RtrFlag is_rtr, BrsFlag is_brs,
                   EsiFlag is_esi);

        /**
         * BRS only is randomized
         */
        FrameFlags(FrameType is_fdf, IdentifierType is_ide, RtrFlag is_rtr, EsiFlag is_esi);

        /**
         * IDE is randomized only
         */
        FrameFlags(FrameType is_fdf, RtrFlag is_rtr, BrsFlag is_brs, EsiFlag is_esi);

        /**
         *  BRS and ESI are randomized (if frame is CAN FD frame)
         */
        FrameFlags(FrameType is_fdf, IdentifierType is_ide, RtrFlag is_rtr);

        /**
         * Randomizes RTR flag, BRS and ESI
         */
        FrameFlags(FrameType is_fdf, IdentifierType is_ide);

        /**
         * Randomizes IDE, BRS, ESI
         */
        FrameFlags(FrameType is_fdf, RtrFlag is_rtr);

        /**
         * Randomizes IDE, BRS
         */
        FrameFlags(FrameType is_fdf, RtrFlag is_rtr, EsiFlag is_esi);

        /**
         * Randomizes IDE, BRS, ESI, RTR.
         */
        FrameFlags(FrameType is_fdf);

        /**
         * Randomizes FDF, BRS, ESI, RTR
         */
        FrameFlags(IdentifierType is_ide);

        /**
         * Randomizes ESI, IDE, RTR
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
         * Operators on comparisons of frame flags
         */
        bool operator==(const FrameFlags rhs);
        bool operator!=(const FrameFlags rhs);

        // Getters
        inline FrameType is_fdf() { return is_fdf_; };
        inline IdentifierType is_ide() { return is_ide_; };
        inline RtrFlag is_rtr() { return is_rtr_; };
        inline BrsFlag is_brs() { return is_brs_; };
        inline EsiFlag is_esi() { return is_esi_; };

        // Setters
        void set_fdf(FrameType is_fdf);
        void set_ide(IdentifierType is_ide);
        void set_rtr(RtrFlag is_rtr);
        void set_brs(BrsFlag is_brs);
        void set_esi(EsiFlag is_esi);

    private:

        /**
         * Corrects frame flag combinations to valid values. E.g. CAN FD and RTR
         * flags can't be set at once (CAN FD frames have no RTR flags). Similarly
         * CAN 2.0 frames have no BRS flags or ESI Flags.
         */
        void CorrectFlags();

        /* CAN frame flags */
        FrameType is_fdf_ = FrameType::Can2_0;
        IdentifierType is_ide_ = IdentifierType::Base;
        RtrFlag is_rtr_ = RtrFlag::DataFrame;
        BrsFlag is_brs_ = BrsFlag::DontShift;
        EsiFlag is_esi_ = EsiFlag::ErrorActive;

        /* Randomization attributes */
        bool randomize_fdf_ = false;
        bool randomize_ide_ = false;
        bool randomize_rtr_ = false;
        bool randomize_brs_ = false;
        bool randomize_esi_ = false;

};

#endif