/***************************************************************************
*   Copyright (C) 2004 by Bo Peng                                         *
*   bpeng@rice.edu
*                                                                         *
*   $LastChangedDate$
*   $Rev$                                                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef _MIGRATOR_H
#define _MIGRATOR_H
/**
   \file
   \brief head file of class migrator:public baseOperator
 */
#include "operator.h"
#include <list>
using std::list;

#include <iostream>
using std::cout;
using std::endl;

#include <algorithm>
using std::sort;
using std::random_shuffle;

#include "simuPOP_cfg.h"
#include <math.h>
// using std::sqrt;

#include "virtualSubPop.h"

namespace simuPOP {


/** This operator migrates individuals from (virtual) subpopulations to other
 *  subpopulations, according to either pre-specified destination
 *  subpopulation stored in an information field, or randomly according to a
 *  migration matrix.
 *  
 *  In the former case, values in a specified information field (default to
 *  \e migrate_to) are considered as destination subpopulation for each
 *  individual. If \e subPops is given, only individuals in specified (virtual)
 *  subpopulations will be migrated where others will stay in their original
 *  subpopulation. Negative values are not allowed in this information field
 *  because they do not represent a valid destination subpopulation ID.
 *
 *  In the latter case, a migration matrix is used to randomly assign
 *  destination subpoulations to each individual. The elements in this matrix
 *  can be probabilities to migrate, proportions of individuals to migrate, or
 *  exact number of individuals to migrate. 
 *
 *  By default, the migration matrix should have \c m by \c m elements if there
 *  are \c m subpopulations. Element <tt>(i, j)</tt> in this matrix represents
 *  migration probability, rate or count from subpopulation \c i to \c j. If
 *  \e subPops (length \c m) and/or \e toSubPops (length \c n) are given,
 *  the matrix should have \c m by \c n elements, corresponding to specified 
 *  source and destination subpopulations. Subpopulations in \e subPops can
 *  be virtual subpopulations, which makes it possible to migrate, for example,
 *  males and females at different rates from a subpopulation. If a
 *  subpopulation in \e toSubPops does not exist, it will be created. In case
 *  that all individuals from a subpopulation are migrated, the empty
 *  subpopulation will be kept.
 *  
 *  If migration is applied by probability, the row of the migration matrix
 *  corresponding to a source subpopulation is intepreted as probabilities to
 *  migrate to each destination subpopulation. Each individual's detination
 *  subpopulation is assigned randomly according to these probabilities. Note
 *  that the probability of staying at the present subpopulation is
 *  automatically calculated so the corresponding matrix elements are ignored.
 *
 *  If migration is applied by proportion, the row of the migration matrix
 *  corresponding to a source subpopulation is intepreted as proportions to
 *  migrate to each destination subpopulation. The number of migrants to each
 *  destination subpopulation is determined before random indidividuals are
 *  chosen to migrate.
 *
 *  If migration is applied by counts, the row of the migration matrix
 *  corresponding to a source subpopulation is intepreted as number of
 *  individuals to migrate to each detination subpopulation. The migrants are
 *  chosen randomly.
 *  
 *  This operator goes through all source (virtual) subpopulations and assign
 *  detination subpopulation of each individual to an information field. An
 *  \c RuntimeError will be raised if an individual is assigned to migrate
 *  more than once. This might happen if you are migrating from two overlapping
 *  virtual subpopulations.
 */
class migrator : public baseOperator
{
public:
	/** Create a migrator that moves individuals from source (virtual)
	 *  subpopulations \e subPops (default to migrate from all subpopulations)
	 *  to destination subpopulations \e toSubPops (default to all
	 *  subpopulations), according to existing values in an information field
	 *  \e infoFields[0], or randomly according to a migration matrix \e rate.
	 *  In the latter case, the size of the matrix should match the number of
	 *  source and destination subpopulations.
	 *
	 *  Depending on the value of parameter \e mode, elements in the migration
	 *  matrix (\e rate) are interpreted as either the probabilities to migrate
	 *  from source to destination subpopulations (\e mode = \c ByProbability),
	 *  proportions of individuals in the source (virtual) subpopulations to
	 *  the destination subpopulations (\e mode = \c ByProportion), numbers
	 *  of migrants in the source (virtual) subpopulations (\e mode
	 *  = \c ByCounts), or ignored completely (\e mode = \c ByIndInfo).
	 *  In the last case, parameter \e subPops is respected (only individuals
	 *  in specified (virtual) subpopulations will migrate) but \e toSubPops
	 *  is ignored.
	 */
	migrator(const matrix & rate = matrix(), int mode = ByProbability, const uintList & toSubPops = uintList(),
		int stage = PreMating, int begin = 0, int end = -1, int step = 1, const intList & at = intList(),
		const repList & rep = repList(), const subPopList & subPops = subPopList(),
		const vectorstr & infoFields = vectorstr(1, "migrate_to"));

	/// destructor
	virtual ~migrator()
	{
	};

	/// deep copy of a migrator
	virtual baseOperator * clone() const
	{
		return new migrator(*this);
	}


	/// return migration rate
	matrix rate()
	{
		return m_rate;
	}


	/** CPPONLY  set migration rate. 
	 */
	void setRates(int mode, const subPopList & fromSubPops, const vectorlu & toSubPops);

	/// apply the migrator to populaiton \e pop.
	virtual bool apply(population & pop);

	/// used by Python print function to print out the general information of the \c migrator
	virtual string __repr__()
	{
		return "<simuPOP::migrator>" ;
	}


protected:
	/// migration rate. its meaning is controled by m_mode
	matrix m_rate;

	/// asProbability (1), asProportion (2), or asCounts.
	int m_mode;

	/// from->to subPop index.
	/// default to 0 - rows of rate - 1, 0 - columns of rate - 1
	vectorlu m_to;
};


/// split a subpopulation
/**
   <funcForm>SplitSubPop</funcForm>
 */
class splitSubPop : public baseOperator
{

public:
	/// split a subpopulation
	/**
	   Split a subpopulation by sizes or proportions. Individuals are randomly (by default)
	   assigned to the resulting subpopulations. Because mating schemes may introduce
	   certain order to individuals, randomization ensures that split subpopulations have
	   roughly even distribution of genotypes.

	   \param which which subpopulation to split. If there is no subpopulation structure,
	    use \c 0 as the first (and only) subpopulation.
	   \param sizes new subpopulation sizes. The sizes should be added up to the original
	    subpopulation (subpopulation \c which) size.
	   \param proportions proportions of new subpopulations. Should be added up to \c 1.
	   \param randomize Whether or not randomize individuals before population split. Default
	    to True.
	 */
	splitSubPop(UINT which = 0,  vectorlu sizes = vectorlu(), vectorf proportions = vectorf(),
		bool randomize = true,
		int stage = PreMating, int begin = 0, int end = -1, int step = 1, const intList & at = intList(),
		const repList & rep = repList(), const subPopList & subPops = subPopList(), const vectorstr & infoFields = vectorstr(1, "migrate_to"))
		: baseOperator("", stage, begin, end, step, at, rep, subPops, infoFields),
		m_which(which), m_subPopSizes(sizes), m_proportions(proportions),
		m_randomize(randomize)
	{
		DBG_FAILIF(sizes.empty() && proportions.empty(), ValueError,
			"Please specify one of subPop and proportions.");
		DBG_FAILIF(!sizes.empty() && !proportions.empty(), ValueError,
			"Please specify only one of subPop and proportions.");
	}


	/// destructor
	virtual ~splitSubPop()
	{
	}


	/// deep copy of a \c splitSubPop operator
	virtual baseOperator * clone() const
	{
		return new splitSubPop(*this);
	}


	/// apply a \c splitSubPop operator
	virtual bool apply(population & pop);

	/// used by Python print function to print out the general information of the \c splitSubPop operator
	virtual string __repr__()
	{
		return "<simuPOP::split population>" ;
	}


private:
	/// which subpop to split
	UINT m_which;

	/// new subpopulation size
	vectorlu m_subPopSizes;

	/// new subpopulation proportions.
	vectorf m_proportions;

	/// random split
	/// randomize population before split.
	/// this is because some mating schemes generate
	/// individuals non-randomly, for example,
	/// put affected individuals at the beginning.
	bool m_randomize;
};

///  merge subpopulations
/**
   This operator merges subpopulations \c subPops to a
   single subpopulation. If \c subPops is ignored, all subpopulations will be merged.
   <funcForm>MergeSubPops</funcForm>
 */
class mergeSubPops : public baseOperator
{

public:
	/// merge subpopulations
	/**
	   \param subPops subpopulations to be merged. Default to all.
	 */
	mergeSubPops(const subPopList & subPops = subPopList(),
		int stage = PreMating, int begin = 0, int end = -1, int step = 1, const intList & at = intList(),
		const repList & rep = repList(), const vectorstr & infoFields = vectorstr())
		: baseOperator("", stage, begin, end, step, at, rep, subPops, infoFields)
	{
	}


	/// destructor
	virtual ~mergeSubPops()
	{
	}


	/// deep copy of a \c mergeSubPops operator
	virtual baseOperator * clone() const
	{
		return new mergeSubPops(*this);
	}


	/// apply a \c mergeSubPops operator
	virtual bool apply(population & pop)
	{
		subPopList sp = applicableSubPops();
		vectoru subPops(sp.size());

		for (size_t i = 0; i < sp.size(); ++i)
			subPops[i] = sp[i].subPop();
		pop.mergeSubPops(subPops);
		return true;
	}


	/// used by Python print function to print out the general information of the \c mergeSubPops operator
	virtual string __repr__()
	{
		return "<simuPOP::merge subpopulations>" ;
	}


};


/// resize subpopulations
/**
   This operator resize subpopulations \c subPops to a
   another size. If \c subPops is ignored, all subpopulations will be resized.
   If the new size is smaller than the original one, the remaining individuals
   are discarded. If the new size if greater, individuals will be copied
   again if propagate is true, and be empty otherwise.
   <funcForm>ResizeSubPops</funcForm>
 */
class resizeSubPops : public baseOperator
{

public:
	/// resize subpopulations
	/**
	   \param newSizes of the specified (or all) subpopulations.
	   \param subPop subpopulations to be resized. Default to all.
	   \param propagate if true (default) and the new size if greater than
	    the original size, individuals will be copied over.
	 */
	resizeSubPops(vectorlu newSizes = vectorlu(), bool propagate = true,
		int stage = PreMating, int begin = 0, int end = -1, int step = 1, const intList & at = intList(),
		const repList & rep = repList(), const subPopList & subPops = subPopList(), const vectorstr & infoFields = vectorstr())
		: baseOperator("", stage, begin, end, step, at, rep, subPops, infoFields),
		m_newSizes(newSizes), m_propagate(propagate)
	{
		DBG_FAILIF(!subPops.empty() && subPops.size() != newSizes.size(), ValueError,
			"Please specify new sizes for each specified subpopulation");
	}


	/// destructor
	virtual ~resizeSubPops()
	{
	}


	/// deep copy of a \c resizeSubPops operator
	virtual baseOperator * clone() const
	{
		return new resizeSubPops(*this);
	}


	/// apply a \c resizeSubPops operator
	virtual bool apply(population & pop);


	/// used by Python print function to print out the general information of the \c resizeSubPops operator
	virtual string __repr__()
	{
		return "<simuPOP::resize subpopulations>" ;
	}


private:
	///
	vectorlu m_newSizes;

	///
	bool m_propagate;
};

}
#endif
