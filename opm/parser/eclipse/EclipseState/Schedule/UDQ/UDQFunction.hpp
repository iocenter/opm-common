/*
  Copyright 2019 Equinor ASA.

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

#ifndef UDQFUNCTION_HPP
#define UDQFUNCTION_HPP

#include <stdexcept>
#include <vector>
#include <functional>
#include <random>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQSet.hpp>

namespace Opm {


class UDQFunction {
public:
    explicit UDQFunction(const std::string& name);
    virtual ~UDQFunction() = default;
    const std::string& name() const;
private:
    std::string m_name;
};

class UDQScalarFunction : public UDQFunction {
public:
    UDQScalarFunction(const std::string&name, std::function<UDQScalar(const UDQSet& arg)> f);
    UDQScalar eval(const UDQSet& arg) const;

    static UDQScalar SUM(const UDQSet& arg);
    static UDQScalar AVEA(const UDQSet& arg);
    static UDQScalar AVEG(const UDQSet& arg);
    static UDQScalar AVEH(const UDQSet& arg);
    static UDQScalar MIN(const UDQSet& arg);
    static UDQScalar MAX(const UDQSet& arg);
    static UDQScalar NORM1(const UDQSet& arg);
    static UDQScalar NORM2(const UDQSet& arg);
    static UDQScalar NORMI(const UDQSet& arg);
    static UDQScalar PROD(const UDQSet& arg);

private:
    std::function<UDQScalar(const UDQSet& arg)> func;
};


class UDQUnaryElementalFunction : public UDQFunction {
public:
    UDQUnaryElementalFunction(const std::string&name, std::function<UDQSet(const UDQSet& arg)> f);
    UDQSet eval(const UDQSet& arg) const;

    static UDQSet ABS(const UDQSet& arg);
    static UDQSet DEF(const UDQSet& arg);
    static UDQSet EXP(const UDQSet& arg);
    static UDQSet IDV(const UDQSet& arg);
    static UDQSet LN(const UDQSet& arg);
    static UDQSet LOG(const UDQSet& arg);
    static UDQSet NINT(const UDQSet& arg);
    static UDQSet SORTA(const UDQSet& arg);
    static UDQSet SORTD(const UDQSet& arg);
    static UDQSet UNDEF(const UDQSet& arg);

    static UDQSet RANDN(std::mt19937& rng, const UDQSet& arg);
    static UDQSet RANDU(std::mt19937& rng, const UDQSet& arg);

private:
    std::function<UDQSet(const UDQSet& arg)> func;
};


class UDQBinaryFunction : public UDQFunction {
public:
    UDQBinaryFunction(const std::string& name, std::function<UDQSet(const UDQSet& lhs, const UDQSet& rhs)> f);
    UDQSet eval(const UDQSet&, const UDQSet& arg) const;

    static UDQSet EQ(double eps, const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet NE(double eps, const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet LE(double eps, const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet GE(double eps, const UDQSet& lhs, const UDQSet& rhs);

    static UDQSet POW(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet LT(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet GT(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet ADD(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet MUL(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet SUB(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet DIV(const UDQSet& lhs, const UDQSet& rhs);

    static UDQSet UADD(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet UMUL(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet UMAX(const UDQSet& lhs, const UDQSet& rhs);
    static UDQSet UMIN(const UDQSet& lhs, const UDQSet& rhs);
private:

    std::function<UDQSet(const UDQSet& lhs, const UDQSet& rhs)> func;
};
}
#endif