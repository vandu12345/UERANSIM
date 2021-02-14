//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "common_types.hpp"
#include <iomanip>
#include <sstream>

Supi Supi::Parse(const std::string &supi)
{
    if (supi[0] == 'i' && supi[1] == 'm' && supi[2] == 's' && supi[3] == 'i' && supi[4] == '-')
    {
        std::string val = supi.substr(5);
        if (val.size() != 15 && val.size() != 16)
            throw std::runtime_error("invalid IMSI value");
        for (char c : val)
            if (c < '0' || c > '9')
                throw std::runtime_error("invalid IMSI value");
        return Supi{"imsi", val};
    }
    throw std::runtime_error("invalid SUPI value");
}

std::string Supi::toString() const
{
    return type + "-" + value;
}

std::string Plmn::toString() const
{
    std::stringstream ss{};
    ss << std::setfill('0') << std::setw(3) << mcc << "/";
    ss << std::setfill('0') << std::setw(isLongMnc ? 3 : 2) << mnc;
    return ss.str();
}
