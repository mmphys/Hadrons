/*
 * Guesser.hpp, part of Hadrons (https://github.com/aportelli/Hadrons)
 *
 * Copyright (C) 2015 - 2020
 *
 * Author: Antonin Portelli <antonin.portelli@me.com>
 *
 * Hadrons is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hadrons is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hadrons.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See the full license in the file "LICENSE" in the top level distribution 
 * directory.
 */

/*  END LEGAL */
#ifndef Hadrons_MSolver_Guesser_hpp_
#define Hadrons_MSolver_Guesser_hpp_

#include <Hadrons/Global.hpp>
#include <Hadrons/EigenPack.hpp>

BEGIN_HADRONS_NAMESPACE
BEGIN_MODULE_NAMESPACE(MSolver)

template <typename FImpl, int nBasis = HADRONS_DEFAULT_LANCZOS_NBASIS>
std::shared_ptr<LinearFunction<typename FImpl::FermionField>> 
makeGuesser(const std::string epackName)
{
    typedef typename FImpl::FermionField                  FermionField;
    typedef BaseFermionEigenPack<FImpl>                   EPack;
    typedef CoarseFermionEigenPack<FImpl, nBasis>         CoarseEPack;
    typedef DeflatedGuesser<FermionField>                 FineGuesser;
    typedef LocalCoherenceDeflatedGuesser<
        FermionField, typename CoarseEPack::CoarseField>  CoarseGuesser;

    std::shared_ptr<LinearFunction<typename FImpl::FermionField>> guesserPt;

    DEFINE_ENV_LAMBDA;

    if (epackName.empty())
    {
        guesserPt.reset(new ZeroGuesser<FermionField>());
    }
    else
    {
        try
        {
            auto &epack = envGetDerived(EPack, CoarseEPack, epackName);
            
            LOG(Message) << "using low-mode deflation with coarse eigenpack '"
                         << epackName << "' (" 
                         << epack.evecCoarse.size() << " modes)" << std::endl;
            guesserPt.reset(new CoarseGuesser(epack.evec, epack.evecCoarse,
                                              epack.evalCoarse));
        }
        catch (Exceptions::ObjectType &e)
        {
            auto &epack = envGet(EPack, epackName);

            LOG(Message) << "using low-mode deflation with eigenpack '"
                         << epackName << "' (" 
                         << epack.evec.size() << " modes)" << std::endl;
            guesserPt.reset(new FineGuesser(epack.evec, epack.eval));
        }
    }

    return guesserPt;
}

END_MODULE_NAMESPACE
END_HADRONS_NAMESPACE

#endif
