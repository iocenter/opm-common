/*
  Copyright 2016 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <string>
#include <vector>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>


namespace Opm {

    WellProductionProperties::
    WellProductionProperties() : predictionMode( true )
    {}

    WellProductionProperties::
    WellProductionProperties( const DeckRecord& record ) :
        OilRate( record.getItem( "ORAT" ).getSIDouble( 0 ) ),
        WaterRate( record.getItem( "WRAT" ).getSIDouble( 0 ) ),
        GasRate( record.getItem( "GRAT" ).getSIDouble( 0 ) )
    {}


    WellProductionProperties WellProductionProperties::history(const WellProductionProperties& prev_properties,
                                                               const DeckRecord& record,
                                                               const WellProducer::ControlModeEnum controlModeWHISTCL,
                                                               const bool switching_from_injector)
    {
        WellProductionProperties p(record);
        p.predictionMode = false;

        // update LiquidRate
        p.LiquidRate = p.WaterRate + p.OilRate;

        if ( record.getItem( "BHP" ).hasValue(0) )
            p.BHPH = record.getItem("BHP").getSIDouble(0);
        if ( record.getItem( "THP" ).hasValue(0) )
            p.THPH = record.getItem("THP").getSIDouble(0);

        const auto& cmodeItem = record.getItem("CMODE");
        if ( cmodeItem.defaultApplied(0) ) {
            const std::string msg = "control mode can not be defaulted for keyword WCONHIST";
            throw std::invalid_argument(msg);
        }

        namespace wp = WellProducer;
        auto cmode = wp::ControlModeFromString( cmodeItem.getTrimmedString( 0 ) );

        // when there is an effective control mode specified by WHISTCL, we always use this control mode
        if (effectiveHistoryProductionControl(controlModeWHISTCL) ) {
            cmode = controlModeWHISTCL;
        }

        if (effectiveHistoryProductionControl(cmode) ) {
            p.addProductionControl( cmode );
            p.controlMode = cmode;
        } else {
            const std::string cmode_string = cmodeItem.getTrimmedString( 0 );
            const std::string msg = "unsupported control mode " + cmode_string + " for WCONHIST";
            throw std::invalid_argument(msg);
        }

        // always have a BHP control/limit, while the limit value needs to be determined
        // the control mode added above can be a BHP control or a type of RATE control
        if ( !p.hasProductionControl( wp::BHP ) )
            p.addProductionControl( wp::BHP );

        if (cmode == wp::BHP) {
            p.setBHPLimit(p.BHPH);
        } else {
            // when the well is switching to history matching producer from prediction mode
            // or switching from injector to producer
            // or switching from BHP control to RATE control (under history matching mode)
            // we use the defaulted BHP limit, otherwise, we use the previous BHP limit
            if ( prev_properties.predictionMode || switching_from_injector
              || prev_properties.controlMode == wp::BHP ) {
                p.resetDefaultBHPLimit();
            } else {
                p.setBHPLimit(prev_properties.getBHPLimit());
            }
        }

        p.VFPTableNumber = record.getItem("VFPTable").get< int >(0);

        if (p.VFPTableNumber == 0)
            p.VFPTableNumber = prev_properties.VFPTableNumber;

        p.ALQValue       = record.getItem("Lift").get< double >(0); //NOTE: Unit of ALQ is never touched

        if (p.ALQValue == 0.)
            p.ALQValue = prev_properties.ALQValue;

        return p;
    }



    WellProductionProperties WellProductionProperties::prediction( const DeckRecord& record, bool addGroupProductionControl)
    {
        WellProductionProperties p(record);
        p.predictionMode = true;

        p.LiquidRate     = record.getItem("LRAT"     ).getSIDouble(0);
        p.ResVRate       = record.getItem("RESV"     ).getSIDouble(0);
        p.BHPLimit       = record.getItem("BHP"      ).getSIDouble(0);
        p.THPLimit       = record.getItem("THP"      ).getSIDouble(0);
        p.ALQValue       = record.getItem("ALQ"      ).get< double >(0); //NOTE: Unit of ALQ is never touched
        p.VFPTableNumber = record.getItem("VFP_TABLE").get< int >(0);

        namespace wp = WellProducer;
        using mode = std::pair< const std::string, wp::ControlModeEnum >;
        static const mode modes[] = {
            { "ORAT", wp::ORAT }, { "WRAT", wp::WRAT }, { "GRAT", wp::GRAT },
            { "LRAT", wp::LRAT }, { "RESV", wp::RESV }, { "THP", wp::THP }
        };

        for( const auto& cmode : modes ) {
            if( !record.getItem( cmode.first ).defaultApplied( 0 ) ) {

                // a zero value THP limit will not be handled as a THP limit
                if (cmode.first == "THP" && p.THPLimit == 0.)
                    continue;

                p.addProductionControl( cmode.second );
            }
        }

        // There is always a BHP constraint, when not specified, will use the default value
        p.addProductionControl( wp::BHP );

        if (addGroupProductionControl) {
            p.addProductionControl(WellProducer::GRUP);
        }


        {
            const auto& cmodeItem = record.getItem("CMODE");
            if (cmodeItem.hasValue(0)) {
                const WellProducer::ControlModeEnum cmode = WellProducer::ControlModeFromString( cmodeItem.getTrimmedString(0) );

                if (p.hasProductionControl( cmode ))
                    p.controlMode = cmode;
                else
                    throw std::invalid_argument("Setting CMODE to unspecified control");
            }
        }
        return p;
    }


    bool WellProductionProperties::operator==(const WellProductionProperties& other) const {
        return OilRate              == other.OilRate
            && WaterRate            == other.WaterRate
            && GasRate              == other.GasRate
            && LiquidRate           == other.LiquidRate
            && ResVRate             == other.ResVRate
            && BHPLimit             == other.BHPLimit
            && THPLimit             == other.THPLimit
            && BHPH                 == other.BHPH
            && THPH                 == other.THPH
            && VFPTableNumber       == other.VFPTableNumber
            && controlMode          == other.controlMode
            && m_productionControls == other.m_productionControls
            && this->predictionMode == other.predictionMode;
    }


    bool WellProductionProperties::operator!=(const WellProductionProperties& other) const {
        return !(*this == other);
    }


    std::ostream& operator<<( std::ostream& stream, const WellProductionProperties& wp ) {
        return stream
            << "WellProductionProperties { "
            << "oil rate: "     << wp.OilRate           << ", "
            << "water rate: "   << wp.WaterRate         << ", "
            << "gas rate: "     << wp.GasRate           << ", "
            << "liquid rate: "  << wp.LiquidRate        << ", "
            << "ResV rate: "    << wp.ResVRate          << ", "
            << "BHP limit: "    << wp.BHPLimit          << ", "
            << "THP limit: "    << wp.THPLimit          << ", "
            << "BHPH: "         << wp.BHPH              << ", "
            << "THPH: "         << wp.THPH              << ", "
            << "VFP table: "    << wp.VFPTableNumber    << ", "
            << "ALQ: "          << wp.ALQValue          << ", "
            << "prediction: "   << wp.predictionMode    << " }";
    }

    bool WellProductionProperties::effectiveHistoryProductionControl(const WellProducer::ControlModeEnum cmode) {
        // Note, not handling CRAT for now
        namespace wp = WellProducer;
        return ( (cmode == wp::LRAT || cmode == wp::RESV || cmode == wp::ORAT ||
                  cmode == wp::WRAT || cmode == wp::GRAT || cmode == wp::BHP) );
    }

    void WellProductionProperties::resetDefaultBHPLimit() {
        BHPLimit = 1. * unit::atm;
    }

    void WellProductionProperties::setBHPLimit(const double limit) {
        BHPLimit = limit;
    }

    double WellProductionProperties::getBHPLimit() const {
        return BHPLimit;
    }

} // namespace Opm
