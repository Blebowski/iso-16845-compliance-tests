#ifndef FRAME_FLAGS_H
#define FRAME_FLAGS_H
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

#include "can.h"

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
        FrameFlags(FrameKind is_fdf, IdentKind is_ide, RtrFlag is_rtr, BrsFlag is_brs,
                   EsiFlag is_esi);

        /**
         * BRS only is randomized
         */
        FrameFlags(FrameKind is_fdf, IdentKind is_ide, RtrFlag is_rtr, EsiFlag is_esi);

        /**
         * IDE is randomized only
         */
        FrameFlags(FrameKind is_fdf, RtrFlag is_rtr, BrsFlag is_brs, EsiFlag is_esi);

        /**
         *  BRS and ESI are randomized (if frame is CAN FD frame)
         */
        FrameFlags(FrameKind is_fdf, IdentKind is_ide, RtrFlag is_rtr);

        /**
         * Randomizes RTR flag, BRS and ESI
         */
        FrameFlags(FrameKind is_fdf, IdentKind is_ide);

        /**
         * Randomizes IDE, BRS, ESI
         */
        FrameFlags(FrameKind is_fdf, RtrFlag is_rtr);

        /**
         * Randomizes IDE, BRS
         */
        FrameFlags(FrameKind is_fdf, RtrFlag is_rtr, EsiFlag is_esi);

        /**
         * Randomizes IDE, BRS, ESI, RTR.
         */
        FrameFlags(FrameKind is_fdf);

        /**
         * Randomizes FDF, BRS, ESI, RTR
         */
        FrameFlags(IdentKind is_ide);

        /**
         * Randomizes ESI, IDE, RTR
         */
        FrameFlags(FrameKind is_fdf, BrsFlag is_brs);

        /**
         * Randomizes RTR and IDE
         */
        FrameFlags(FrameKind is_fdf, BrsFlag is_brs, EsiFlag is_esi);

        /**
         * Randomizes BRS, RTR, IDE
         */
        FrameFlags(FrameKind is_fdf, EsiFlag is_esi);

        /**
         * Randomizes RTR, BRS
         */
        FrameFlags(FrameKind is_fdf, IdentKind is_ide, EsiFlag is_esi);

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
        inline FrameKind is_fdf() const {
            return is_fdf_;
        };

        inline IdentKind is_ide() const {
            return is_ide_;
        };

        inline RtrFlag is_rtr() const {
            return is_rtr_;
        };

        inline BrsFlag is_brs() const {
            return is_brs_;
        };

        inline EsiFlag is_esi() const {
            return is_esi_;
        };

        // Setters
        void set_fdf(FrameKind is_fdf);
        void set_ide(IdentKind is_ide);
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
        FrameKind is_fdf_ = FrameKind::Can20;
        IdentKind is_ide_ = IdentKind::Base;
        RtrFlag is_rtr_ = RtrFlag::Data;
        BrsFlag is_brs_ = BrsFlag::NoShift;
        EsiFlag is_esi_ = EsiFlag::ErrAct;

        /* Randomization attributes */
        bool randomize_fdf_ = false;
        bool randomize_ide_ = false;
        bool randomize_rtr_ = false;
        bool randomize_brs_ = false;
        bool randomize_esi_ = false;
};

#endif