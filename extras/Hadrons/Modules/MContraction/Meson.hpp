/*************************************************************************************

Grid physics library, www.github.com/paboyle/Grid 

Source file: extras/Hadrons/Modules/MContraction/Meson.hpp

Copyright (C) 2015
Copyright (C) 2016
Copyright (C) 2017

Author: Antonin Portelli <antonin.portelli@me.com>
        Andrew Lawson    <andrew.lawson1991@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

See the full license in the file "LICENSE" in the top level distribution directory
*************************************************************************************/
/*  END LEGAL */

#ifndef Hadrons_MContraction_Meson_hpp_
#define Hadrons_MContraction_Meson_hpp_

#include <Grid/Hadrons/Global.hpp>
#include <Grid/Hadrons/Module.hpp>
#include <Grid/Hadrons/ModuleFactory.hpp>

BEGIN_HADRONS_NAMESPACE

/*
 
 Meson contractions
 -----------------------------
 
 * options:
 - q1: input propagator 1 (string)
 - q2: input propagator 2 (string)
 - gammas: gamma products to insert at sink & source, pairs of gamma matrices 
           (space-separated strings) in angled brackets (i.e. <g_sink g_src>),
           in a sequence (e.g. "<Gamma5 Gamma5><Gamma5 GammaT>").

           Special values: "all" - perform all possible contractions.
 - sink: module to compute the sink to use in contraction (string).
*/

/******************************************************************************
 *                                TMeson                                       *
 ******************************************************************************/
BEGIN_MODULE_NAMESPACE(MContraction)

typedef std::pair<Gamma::Algebra, Gamma::Algebra> GammaPair;

class MesonPar: Serializable
{
public:
    GRID_SERIALIZABLE_CLASS_MEMBERS(MesonPar,
                                    std::string, q1,
                                    std::string, q2,
                                    std::string, gammas,
                                    std::string, sink,
                                    std::string, output);
};

template <typename FImpl1, typename FImpl2>
class TMeson: public Module<MesonPar>
{
public:
    FERM_TYPE_ALIASES(FImpl1, 1);
    FERM_TYPE_ALIASES(FImpl2, 2);
    FERM_TYPE_ALIASES(ScalarImplCR, Scalar);
    SINK_TYPE_ALIASES(Scalar);
    class Result: Serializable
    {
    public:
        GRID_SERIALIZABLE_CLASS_MEMBERS(Result,
                                        Gamma::Algebra, gamma_snk,
                                        Gamma::Algebra, gamma_src,
                                        std::vector<Complex>, corr);
    };
public:
    // constructor
    TMeson(const std::string name);
    // destructor
    virtual ~TMeson(void) = default;
    // dependencies/products
    virtual std::vector<std::string> getInput(void);
    virtual std::vector<std::string> getOutput(void);
    virtual void parseGammaString(std::vector<GammaPair> &gammaList);
    // execution
    virtual void execute(void);
};

MODULE_REGISTER_NS(Meson, ARG(TMeson<FIMPL, FIMPL>), MContraction);

/******************************************************************************
 *                           TMeson implementation                            *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
template <typename FImpl1, typename FImpl2>
TMeson<FImpl1, FImpl2>::TMeson(const std::string name)
: Module<MesonPar>(name)
{}

// dependencies/products ///////////////////////////////////////////////////////
template <typename FImpl1, typename FImpl2>
std::vector<std::string> TMeson<FImpl1, FImpl2>::getInput(void)
{
    std::vector<std::string> input = {par().q1, par().q2, par().sink};
    
    return input;
}

template <typename FImpl1, typename FImpl2>
std::vector<std::string> TMeson<FImpl1, FImpl2>::getOutput(void)
{
    std::vector<std::string> output = {getName()};
    
    return output;
}

template <typename FImpl1, typename FImpl2>
void TMeson<FImpl1, FImpl2>::parseGammaString(std::vector<GammaPair> &gammaList)
{
    gammaList.clear();
    // Determine gamma matrices to insert at source/sink.
    if (par().gammas.compare("all") == 0)
    {
        // Do all contractions.
        for (unsigned int i = 1; i < Gamma::nGamma; i += 2)
        {
            for (unsigned int j = 1; j < Gamma::nGamma; j += 2)
            {
                gammaList.push_back(std::make_pair((Gamma::Algebra)i, 
                                                   (Gamma::Algebra)j));
            }
        }
    }
    else
    {
        // Parse individual contractions from input string.
        gammaList = strToVec<GammaPair>(par().gammas);
    }
}


// execution ///////////////////////////////////////////////////////////////////
#define mesonConnected(q1, q2, gSnk, gSrc) \
(g5*(gSnk))*(q1)*(adj(gSrc)*g5)*adj(q2)

template <typename FImpl1, typename FImpl2>
void TMeson<FImpl1, FImpl2>::execute(void)
{
    LOG(Message) << "Computing meson contractions '" << getName() << "' using"
                 << " quarks '" << par().q1 << "' and '" << par().q2 << "'"
                 << std::endl;
    
    CorrWriter             writer(par().output);
    std::vector<TComplex>  buf;
    std::vector<Result>    result;
    Gamma                  g5(Gamma::Algebra::Gamma5);
    std::vector<GammaPair> gammaList;
    int                    nt = env().getDim(Tp);
    
    parseGammaString(gammaList);
    result.resize(gammaList.size());
    for (unsigned int i = 0; i < result.size(); ++i)
    {
        result[i].gamma_snk = gammaList[i].first;
        result[i].gamma_src = gammaList[i].second;
        result[i].corr.resize(nt);
    }
    if (env().template isObjectOfType<SlicedPropagator1>(par().q1) and
        env().template isObjectOfType<SlicedPropagator2>(par().q2))
    {
        SlicedPropagator1 &q1 = *env().template getObject<SlicedPropagator1>(par().q1);
        SlicedPropagator2 &q2 = *env().template getObject<SlicedPropagator2>(par().q2);
        
        LOG(Message) << "(propagator already sinked)" << std::endl;
        for (unsigned int i = 0; i < result.size(); ++i)
        {
            Gamma gSnk(gammaList[i].first);
            Gamma gSrc(gammaList[i].second);
            
            for (unsigned int t = 0; t < buf.size(); ++t)
            {
                result[i].corr[t] = TensorRemove(trace(mesonConnected(q1[t], q2[t], gSnk, gSrc)));
            }
        }
    }
    else
    {
        PropagatorField1 &q1   = *env().template getObject<PropagatorField1>(par().q1);
        PropagatorField2 &q2   = *env().template getObject<PropagatorField2>(par().q2);
        LatticeComplex   c(env().getGrid());
        
        LOG(Message) << "(using sink '" << par().sink << "')" << std::endl;
        for (unsigned int i = 0; i < result.size(); ++i)
        {
            Gamma       gSnk(gammaList[i].first);
            Gamma       gSrc(gammaList[i].second);
            std::string ns;
                
            ns = env().getModuleNamespace(env().getObjectModule(par().sink));
            if (ns == "MSource")
            {
                PropagatorField1 &sink =
                    *env().template getObject<PropagatorField1>(par().sink);
                
                c = trace(mesonConnected(q1, q2, gSnk, gSrc)*sink);
                sliceSum(c, buf, Tp);
            }
            else if (ns == "MSink")
            {
                SinkFnScalar &sink = *env().template getObject<SinkFnScalar>(par().sink);
                
                c   = trace(mesonConnected(q1, q2, gSnk, gSrc));
                buf = sink(c);
            }
            for (unsigned int t = 0; t < buf.size(); ++t)
            {
                result[i].corr[t] = TensorRemove(buf[t]);
            }
        }
    }
    write(writer, "meson", result);
}

END_MODULE_NAMESPACE

END_HADRONS_NAMESPACE

#endif // Hadrons_MContraction_Meson_hpp_
