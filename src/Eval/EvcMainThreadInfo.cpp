/*---------------------------------------------------------------------------------*/
/*  NOMAD - Nonlinear Optimization by Mesh Adaptive Direct Search -                */
/*                                                                                 */
/*  NOMAD - Version 4.0.0 has been created by                                      */
/*                 Viviane Rochon Montplaisir  - Polytechnique Montreal            */
/*                 Christophe Tribes           - Polytechnique Montreal            */
/*                                                                                 */
/*  The copyright of NOMAD - version 4.0.0 is owned by                             */
/*                 Charles Audet               - Polytechnique Montreal            */
/*                 Sebastien Le Digabel        - Polytechnique Montreal            */
/*                 Viviane Rochon Montplaisir  - Polytechnique Montreal            */
/*                 Christophe Tribes           - Polytechnique Montreal            */
/*                                                                                 */
/*  NOMAD v4 has been funded by Rio Tinto, Hydro-Québec, NSERC (Natural            */
/*  Sciences and Engineering Research Council of Canada), InnovÉÉ (Innovation      */
/*  en Énergie Électrique) and IVADO (The Institute for Data Valorization)         */
/*                                                                                 */
/*  NOMAD v3 was created and developed by Charles Audet, Sebastien Le Digabel,     */
/*  Christophe Tribes and Viviane Rochon Montplaisir and was funded by AFOSR       */
/*  and Exxon Mobil.                                                               */
/*                                                                                 */
/*  NOMAD v1 and v2 were created and developed by Mark Abramson, Charles Audet,    */
/*  Gilles Couture, and John E. Dennis Jr., and were funded by AFOSR and           */
/*  Exxon Mobil.                                                                   */
/*                                                                                 */
/*  Contact information:                                                           */
/*    Polytechnique Montreal - GERAD                                               */
/*    C.P. 6079, Succ. Centre-ville, Montreal (Quebec) H3C 3A7 Canada              */
/*    e-mail: nomad@gerad.ca                                                       */
/*    phone : 1-514-340-6053 #6928                                                 */
/*    fax   : 1-514-340-5665                                                       */
/*                                                                                 */
/*  This program is free software: you can redistribute it and/or modify it        */
/*  under the terms of the GNU Lesser General Public License as published by       */
/*  the Free Software Foundation, either version 3 of the License, or (at your     */
/*  option) any later version.                                                     */
/*                                                                                 */
/*  This program is distributed in the hope that it will be useful, but WITHOUT    */
/*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or          */
/*  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License    */
/*  for more details.                                                              */
/*                                                                                 */
/*  You should have received a copy of the GNU Lesser General Public License       */
/*  along with this program. If not, see <http://www.gnu.org/licenses/>.           */
/*                                                                                 */
/*  You can find information on the NOMAD software at www.gerad.ca/nomad           */
/*---------------------------------------------------------------------------------*/

#include <unistd.h> // For usleep

#include "../Eval/EvcMainThreadInfo.hpp"
#include "../Output/OutputQueue.hpp"

/*-------------------------*/
/* Class EvcMainThreadInfo */
/*-------------------------*/
void NOMAD::EvcMainThreadInfo::init()
{
    if (nullptr != _evaluator)
    {
        _computeSuccessType.setDefaultComputeSuccessTypeFunction(_evaluator->getEvalType());
    }
}


std::shared_ptr<NOMAD::Evaluator> NOMAD::EvcMainThreadInfo::setEvaluator(std::shared_ptr<NOMAD::Evaluator> evaluator)
{
    auto previousEvaluator = _evaluator;
    _evaluator = evaluator;
    if (nullptr != _evaluator)
    {
        _computeSuccessType.setDefaultComputeSuccessTypeFunction(_evaluator->getEvalType());
    }

    return previousEvaluator;
}


std::shared_ptr<NOMAD::EvalParameters> NOMAD::EvcMainThreadInfo::getEvalParams() const
{
    return (nullptr == _evaluator) ? nullptr : _evaluator->getEvalParams();
}


NOMAD::EvalType NOMAD::EvcMainThreadInfo::getEvalType() const
{
    return (nullptr == _evaluator) ? NOMAD::EvalType::UNDEFINED : _evaluator->getEvalType();
}


bool NOMAD::EvcMainThreadInfo::getOpportunisticEval() const
{
    while (true)
    {
        try
        {
            return _evalContParams->getAttributeValue<bool>("OPPORTUNISTIC_EVAL");
        }
        catch (NOMAD::ParameterToBeChecked &e)
        {
            // Exception due to parameters being in process of checkAndComply().
            // While will loop - Retry
        }
    }
}


void NOMAD::EvcMainThreadInfo::setOpportunisticEval(const bool opportunisticEval)
{
    _evalContParams->setAttributeValue("OPPORTUNISTIC_EVAL", opportunisticEval);
    _evalContParams->checkAndComply();
}


bool NOMAD::EvcMainThreadInfo::getUseCache() const
{
    while (true)
    {
        try
        {
            return _evalContParams->getAttributeValue<bool>("USE_CACHE");
        }
        catch (NOMAD::ParameterToBeChecked &e)
        {
            // Exception due to parameters being in process of checkAndComply().
            // While will loop - Retry
        }
    }
}


void NOMAD::EvcMainThreadInfo::setUseCache(const bool useCache)
{
    _evalContParams->setAttributeValue("USE_CACHE", useCache);
    _evalContParams->checkAndComply();
}


size_t NOMAD::EvcMainThreadInfo::getMaxBbEvalInSubproblem() const
{
    while (true)
    {
        try
        {
            return _evalContParams->getAttributeValue<size_t>("MAX_BB_EVAL_IN_SUBPROBLEM");
        }
        catch (NOMAD::ParameterToBeChecked &e)
        {
            // Exception due to parameters being in process of checkAndComply().
            // While will loop - Retry
        }
    }
}


std::vector<NOMAD::EvalPoint> NOMAD::EvcMainThreadInfo::retrieveAllEvaluatedPoints()
{
    std::vector<NOMAD::EvalPoint> allEvaluatedPoints;

    bool warningShown = false;
    while (_currentlyRunning > 0)
    {
        // Due to race conditions, retrieveAllEvaluatedPoints() might be called while
        // some points are still being evaluated. Wait for all points to be evaluated.
        OUTPUT_INFO_START
        if (!warningShown)
        {
            std::string s = "Warning: Calling retrieveAllEvaluatedPoints() while still ";
            s += NOMAD::itos(_currentlyRunning) + " currently running";
            NOMAD::OutputQueue::Add(s, NOMAD::OutputLevel::LEVEL_INFO);
            warningShown = true;
        }
        OUTPUT_INFO_END
        usleep(10);
    }

    allEvaluatedPoints.insert(allEvaluatedPoints.end(),
                              std::make_move_iterator(_evaluatedPoints.begin()),
                              std::make_move_iterator(_evaluatedPoints.end()));
    _evaluatedPoints.clear();

    return allEvaluatedPoints;
}


void NOMAD::EvcMainThreadInfo::addEvaluatedPoint(const NOMAD::EvalPoint& evaluatedPoint)
{
#ifdef _OPENMP
    #pragma omp critical(addEvaluatedPoint)
#endif // _OPENMP
    {
        _evaluatedPoints.push_back(evaluatedPoint);
    }
}


void NOMAD::EvcMainThreadInfo::setSuccessType(const NOMAD::SuccessType& success)
{
    // Note setSuccessType is already called inside a critical section, so do not
    // add another critical section here.
    _success = success;
}


void NOMAD::EvcMainThreadInfo::incCurrentlyRunning(const size_t n)
{
    _currentlyRunning += n;
}


void NOMAD::EvcMainThreadInfo::decCurrentlyRunning(const size_t n)
{
    if (0 == _currentlyRunning)
    {
        // Note: we don't know the main thread number.
        std::string s = "Error in EvaluatorControl main thread management: Trying to decrease number of currently running evaluations which is already 0";
        throw NOMAD::Exception(__FILE__, __LINE__, s);
    }
    _currentlyRunning -= n;
}


void NOMAD::EvcMainThreadInfo::setStopReason(const NOMAD::EvalStopType& s)
{
    _stopReason.set(s);
}