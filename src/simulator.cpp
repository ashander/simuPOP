/**
 *  $File: Simulator.cpp $
 *  $LastChangedDate$
 *  $Rev$
 *
 *  This file is part of simuPOP, a forward-time population genetics
 *  simulation environment. Please visit http://simupop.sourceforge.net
 *  for details.
 *
 *  Copyright (C) 2004 - 2009 Bo Peng (bpeng@mdanderson.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "simulator.h"

// for file compression
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>

using std::ostringstream;

namespace simuPOP {

Population & pyPopIterator::next()
{
	if (m_index == m_end)
		throw StopIteration("");
	else
		return **(m_index++);
}


Simulator::Simulator(PyObject * pops, UINT rep, bool steal)
{
	DBG_ASSERT(rep >= 1, ValueError,
		"Number of replicates should be greater than or equal one.");

	DBG_DO(DBG_SIMULATOR, cerr << "Creating Simulator " << endl);
	m_pops = vector<Population *>();

	if (PySequence_Check(pops)) {
		UINT size = PySequence_Size(pops);
		for (size_t i = 0; i < size; ++i) {
			PyObject * item = PySequence_GetItem(pops, i);
			void * pop = pyPopPointer(item);
			DBG_ASSERT(pop, ValueError, "Parameter pops should be a single population or a list populations.");
			if (steal) {
				Population * tmp = new Population();
				tmp->swap(*reinterpret_cast<Population *>(pop));
				m_pops.push_back(tmp);
			} else {
				try {
					m_pops.push_back(reinterpret_cast<Population *>(pop)->clone());
					DBG_FAILIF(m_pops.back() == NULL,
						SystemError, "Fail to create new replicate");
				} catch (...) {
					throw RuntimeError("Failed to create a population.");
				}
			}
			Py_DECREF(item);
		}
	} else {
		void * pop = pyPopPointer(pops);
		DBG_ASSERT(pop, ValueError, "Parameter pops should be a single population or a list populations.");
		if (steal) {
			Population * tmp = new Population();
			tmp->swap(*reinterpret_cast<Population *>(pop));
			m_pops.push_back(tmp);
		} else {
			try {
				m_pops.push_back(reinterpret_cast<Population *>(pop)->clone());
				DBG_FAILIF(m_pops.back() == NULL,
					SystemError, "Fail to create new replicate");
			} catch (...) {
				throw RuntimeError("Failed to create a population.");
			}
		}
	}
	// parameter rep
	UINT numRep = m_pops.size();
	for (UINT i = 1; i < rep; ++i) {
		for (UINT j = 0; j < numRep; ++j) {
			try {
				m_pops.push_back(m_pops[j]->clone());
				DBG_FAILIF(m_pops.back() == NULL,
					SystemError, "Fail to create new replicate.");
			} catch (...) {
				throw RuntimeError("Failed to create a population.");
			}
		}
	}
	// set var "rep"
	for (UINT i = 0; i < m_pops.size(); ++i)
		m_pops[i]->setRep(i);
	// create replicates of given Population
	m_scratchPop = new Population();

	DBG_FAILIF(m_scratchPop == NULL,
		SystemError, "Fail to create scratch population");

	// set generation number for all replicates
	DBG_DO(DBG_SIMULATOR, cerr << "Simulator created" << endl);
}


Simulator::~Simulator()
{
	// call the destructor of each replicates
	delete m_scratchPop;

	for (UINT i = 0; i < m_pops.size(); ++i)
		delete m_pops[i];
}


Simulator::Simulator(const Simulator & rhs) :
	m_pops(0),
	m_scratchPop(NULL)
{
	m_scratchPop = rhs.m_scratchPop->clone();
	m_pops = vector<Population *>(rhs.m_pops.size());
	for (size_t i = 0; i < m_pops.size(); ++i) {
		m_pops[i] = rhs.m_pops[i]->clone();
		m_pops[i]->setRep(i);
	}
}


Simulator * Simulator::clone() const
{
	return new Simulator(*this);
}


Population & Simulator::population(UINT rep) const
{
	DBG_FAILIF(rep >= m_pops.size(), IndexError,
		"replicate index out of range. From 0 to numRep()-1 ");

	return *m_pops[rep];
}


Population & Simulator::extract(UINT rep)
{
	DBG_FAILIF(rep >= m_pops.size(), IndexError,
		"replicate index out of range. From 0 to numRep()-1 ");

	Population * pop = m_pops[rep];
	m_pops.erase(m_pops.begin() + rep);
	return *pop;
}


void Simulator::add(const Population & pop, bool steal)
{

	if (steal) {
		Population * tmp = new Population();
		const_cast<Population &>(pop).swap(*tmp);
		m_pops.push_back(tmp);
	} else
		m_pops.push_back(new Population(pop));
	DBG_FAILIF(m_pops.back() == NULL,
		RuntimeError, "Fail to add new Population.");
}


string Simulator::describe(bool format)
{
	return "<simuPOP.Simulator> a simulator with " + toStr(m_pops.size()) + " Population" + (m_pops.size() == 1 ? "." : "s.");
}


vectoru Simulator::evolve(
                          const opList & initOps,
                          const opList & preOps,
                          const MatingScheme & matingScheme,
                          const opList & postOps,
                          const opList & finalOps,
                          int gens)
{
	if (numRep() == 0)
		return vectoru();

	// check compatibility of operators
	for (size_t i = 0; i < preOps.size(); ++i) {
		DBG_ASSERT(preOps[i]->isCompatible(*m_pops[0]), ValueError,
			"Operator " + preOps[i]->describe() + " is not compatible.");
	}
	for (size_t i = 0; i < postOps.size(); ++i) {
		DBG_ASSERT(postOps[i]->isCompatible(*m_pops[0]), ValueError,
			"Operator " + postOps[i]->describe() + " is not compatible.");
	}
	if (!matingScheme.isCompatible(population(0)))
		throw ValueError("mating type is not compatible with current population settings.");

	vector<bool> activeReps(m_pops.size());
	fill(activeReps.begin(), activeReps.end(), true);
	UINT numStopped = 0;

	// evolved generations, which will be returned.
	vectoru evolvedGens(m_pops.size(), 0U);

	// does not evolve.
	if (gens == 0)
		return evolvedGens;


	InitClock();

	// appy pre-op, most likely initializer. Do not check if they are active
	// or if they are successful
	if (!initOps.empty())
		apply(initOps);

	ElapsedTime("PreopDone");

	// make sure rep and gen exists in pop
	for (UINT curRep = 0; curRep < m_pops.size(); curRep++) {
		if (!m_pops[curRep]->getVars().hasVar("gen"))
			m_pops[curRep]->setGen(0);
		m_pops[curRep]->setRep(curRep);
	}

	while (1) {
		// save refcount at the beginning
#ifdef Py_REF_DEBUG
		saveRefCount();
#endif

		for (UINT curRep = 0; curRep < m_pops.size(); curRep++) {
			Population & curPop = *m_pops[curRep];
			int curGen = curPop.gen();
			int end = -1;
			if (gens > 0)
				end = curGen + gens - 1;
			DBG_FAILIF(end < 0 && preOps.empty() && postOps.empty(), ValueError,
				"Evolve with unspecified ending generation should have at least one terminator (operator)");

			DBG_ASSERT(static_cast<int>(curRep) == curPop.rep(), SystemError,
				"Replicate number does not match");

			if (!activeReps[curRep])
				continue;

			size_t it = 0;                                            // asign a value to reduce compiler warning

			if (PyErr_CheckSignals()) {
				cerr << "Evolution stopped due to keyboard interruption." << endl;
				fill(activeReps.begin(), activeReps.end(), false);
				numStopped = activeReps.size();
			}
			// apply pre-mating ops to current gen()
			if (!preOps.empty()) {
				for (it = 0; it < preOps.size(); ++it) {
					if (!preOps[it]->isActive(curRep, curGen, end, activeReps))
						continue;

					try {
						if (!preOps[it]->apply(curPop)) {
							DBG_DO(DBG_SIMULATOR, cerr << "Pre-mating Operator " + preOps[it]->describe() +
								" stops at replicate " + toStr(curRep) << endl);

							if (activeReps[curRep]) {
								numStopped++;
								activeReps[curRep] = false;
								break;
							}
						}
						if (PyErr_CheckSignals())
							throw StopEvolution("Evolution stopped due to keyboard interruption.");
					} catch (StopEvolution e) {
						DBG_DO(DBG_SIMULATOR, cerr	<< "All replicates are stopped due to a StopEvolution exception raised by "
							                        << "Pre-mating Operator " + preOps[it]->describe() +
							" stops at replicate " + toStr(curRep) << endl);
						if (e.message()[0] != '\0')
							cerr << e.message() << endl;
						fill(activeReps.begin(), activeReps.end(), false);
						numStopped = activeReps.size();
						break;
					}
					ElapsedTime("PreMatingOp: " + preOps[it]->describe());
				}
			}

			if (!activeReps[curRep])
				continue;
			// start mating:
			try {
				if (!const_cast<MatingScheme &>(matingScheme).mate(curPop, scratchPopulation())) {
					DBG_DO(DBG_SIMULATOR, cerr << "Mating stops at replicate " + toStr(curRep) << endl);

					numStopped++;
					activeReps[curRep] = false;
					// does not execute post-mating operator
					continue;
				}
				if (PyErr_CheckSignals())
					throw StopEvolution("Evolution stopped due to keyboard interruption.");
			} catch (StopEvolution e) {
				DBG_DO(DBG_SIMULATOR, cerr	<< "All replicates are stopped due to a StopEvolution exception raised by "
					                        << "During-mating Operator at replicate " + toStr(curRep) << endl);
				if (e.message()[0] != '\0')
					cerr << e.message() << endl;
				fill(activeReps.begin(), activeReps.end(), false);
				numStopped = activeReps.size();
				// does not execute post mating operator
				continue;
			}

			ElapsedTime("matingDone");

			// apply post-mating ops to next gen()
			if (!postOps.empty()) {
				for (it = 0; it < postOps.size(); ++it) {
					if (!postOps[it]->isActive(curRep, curGen, end, activeReps))
						continue;

					try {
						if (!postOps[it]->apply(curPop)) {
							DBG_DO(DBG_SIMULATOR, cerr << "Post-mating Operator " + postOps[it]->describe() +
								" stops at replicate " + toStr(curRep) << endl);
							numStopped++;
							activeReps[curRep] = false;
							// does not run the rest of the post-mating operators.
							break;
						}
						if (PyErr_CheckSignals())
							throw StopEvolution("Evolution stopped due to keyboard interruption.");
					} catch (StopEvolution e) {
						DBG_DO(DBG_SIMULATOR, cerr	<< "All replicates are stopped due to a StopEvolution exception raised by "
							                        << "Post-mating Operator " + postOps[it]->describe() +
							" stops at replicate " + toStr(curRep) << endl);
						if (e.message()[0] != '\0')
							cerr << e.message() << endl;
						fill(activeReps.begin(), activeReps.end(), false);
						numStopped = activeReps.size();
						// does not run the rest of the post-mating operators.
						break;
					}
					ElapsedTime("PostMatingOp: " + postOps[it]->describe());
				}
			}
			// if a replicate stops at a post mating operator, consider one evolved generation.
			++evolvedGens[curRep];
			curPop.setGen(curGen + 1);
		}                                                                                       // each replicates

#ifdef Py_REF_DEBUG
		checkRefCount();
#endif

		--gens;
		//
		//   start 0, gen = 2
		//   0 -> 1 -> 2 stop (two generations)
		//
		//   step:
		//    cur, end = cur +1
		//    will go two generations.
		//  therefore, step should:
		if (numStopped == m_pops.size() || gens == 0)
			break;
	}                                                                                         // the big loop

	if (!finalOps.empty())
		apply(finalOps);

	// close every opened file (including append-cross-evolution ones)
	ostreamManager().closeAll();
	return evolvedGens;
}


bool Simulator::apply(const opList & ops)
{
	for (size_t i = 0; i < ops.size(); ++i) {
		// check compatibility of operators
		DBG_ASSERT(ops[i]->isCompatible(*m_pops[0]), ValueError,
			"Operator " + ops[i]->describe() + " is not compatible.");
	}

	// really apply
	for (UINT curRep = 0; curRep < m_pops.size(); curRep++) {
		Population & curPop = *m_pops[curRep];
		size_t it;

		// apply pre-mating ops to current gen
		for (it = 0; it < ops.size(); ++it) {
			vector<bool> activeReps(m_pops.size());
			fill(activeReps.begin(), activeReps.end(), true);
			if (!ops[it]->isActive(curRep, 0, 0, activeReps, true))
				continue;

			ops[it]->apply(curPop);

			ElapsedTime("PrePost-preMatingop" + toStr(it));
		}
	}
	return true;
}


int Simulator::__cmp__(const Simulator & rhs) const
{
	if (numRep() != rhs.numRep())
		return 1;

	for (size_t i = 0; i < numRep(); ++i)
		if (population(i).__cmp__(rhs.population(i)) != 0)
			return 1;

	return 0;
}


string describe(const opList & initOps,
                const opList & preOps,
                const MatingScheme & matingScheme,
                const opList & postOps,
                const opList & finalOps,
                int gen, UINT numRep)
{
	vectorstr allDesc(numRep, "");

	// assuming all active replicates.
	vector<bool> activeReps(numRep);

	for (UINT curRep = 0; curRep < numRep; curRep++) {
		ostringstream desc;

		if (initOps.empty())
			desc << "No operator is used to initialize Population (initOps).\n";
		else {
			desc << "Apply pre-evolution operators to the initial population (initOps).\n<ul>\n";
			for (size_t it = 0; it < initOps.size(); ++it)
				desc << "<li>" << initOps[it]->describe(false) << " " << initOps[it]->applicability(true, false) << endl;
			desc << "</ul>\n";
		}
		if (gen < 0)
			desc << "\nEvolve a population indefinitely until an operator determines it." << endl;
		else
			desc << "\nEvolve a population for " << gen << " generations" << endl;
		desc << "<ul>\n";
		if (preOps.empty())
			desc << "<li>No operator is applied to the parental generation (preOps)." << endl;
		else {
			desc << "<li>Apply pre-mating operators to the parental generation (preOps)\n<ul>\n";
			for (size_t it = 0; it < preOps.size(); ++it)
				if (preOps[it]->isActive(curRep, 0, 0, activeReps, true))
					desc << "<li>" << preOps[it]->describe(false) << " " << preOps[it]->applicability() << endl;
			desc << "</ul>\n";
		}
		desc	<< "\n<li>Populate an offspring populaton from the parental population using mating scheme "
		        << matingScheme.describe(false) << endl;
		//
		if (postOps.empty())
			desc << "\n<li>No operator is applied to the offspring population (postOps)." << endl;
		else {
			desc << "\n<li>Apply post-mating operators to the offspring population (postOps).\n<ul>\n";
			for (size_t it = 0; it < postOps.size(); ++it)
				if (postOps[it]->isActive(curRep, 0, 0, activeReps, true))
					desc << "<li>" << postOps[it]->describe(false) << " " << postOps[it]->applicability() << endl;
			desc << "</ul>\n";
		}
		desc << "</ul>\n\n";
		if (finalOps.empty() )
			desc << "No operator is applied to the final population (finalOps)." << endl;
		else {
			desc << "Apply post-evolution operators (finalOps)\n<ul>\n";
			for (size_t it = 0; it < finalOps.size(); ++it)
				desc << "<li>" << finalOps[it]->describe(false) << " " << finalOps[it]->applicability(true, false) << endl;
			desc << "</ul>\n";
		}
		allDesc[curRep] = desc.str();
	}
	ostringstream desc;
	vectoru reps;
	for (UINT curRep = 0; curRep < numRep; curRep++) {
		if (reps.empty())
			reps.push_back(curRep);
		else {
			if (allDesc[curRep] == allDesc[curRep - 1])
				reps.push_back(curRep);
			else {
				desc << "Replicate";
				for (size_t i = 0; i < reps.size(); ++i)
					desc << " " << i;
				desc << ":\n" << allDesc[curRep - 1] << "\n";
				reps.clear();
				reps.push_back(curRep);
			}
		}
	}
	// reps should not be empty
	desc << "Replicate";
	for (size_t i = 0; i < reps.size(); ++i)
		desc << " " << i;
	desc << ":\n" << allDesc.back();
	return formatText(desc.str());
}


}
