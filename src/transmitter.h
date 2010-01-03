/**
 *  $File: transmitter.h $
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

#ifndef _TRANSMITTER_H
#define _TRANSMITTER_H
/**
   \file
   \brief head file of class Recombinator:public BaseOperator
 */
#include "operator.h"

#include <iterator>
using std::ostream;
using std::ostream_iterator;

namespace simuPOP {

/** This during mating operator is the base class of all genotype transmitters.
 *  It is made available to users because it provides a few member functions
 *  that can be used by derived transmitters, and by customized Python
 *  during mating operators.
 */
class GenoTransmitter : public BaseOperator
{
public:
	/** Create a base genotype transmitter.
	 */
	GenoTransmitter(const stringFunc & output = "", int begin = 0, int end = -1, int step = 1, const intList & at = vectori(),
		const intList & reps = intList(), const subPopList & subPops = subPopList(),
		const stringList & infoFields = vectorstr()) :
		BaseOperator(output, begin, end, step, at, reps, subPops, infoFields),
		m_ploidy(0), m_hasCustomizedChroms(false), m_lociToCopy(0), m_chromIdx(0)
	{
	}


	/// Deep copy of a base genotype transmitter.
	BaseOperator * clone() const
	{
		return new GenoTransmitter(*this);
	}


	/** Clear (set alleles to zero) chromosome \e chrom on the \e ploidy-th
	 *  homologous set of chromosomes of individual \e ind. It is equivalent to
	 *  <tt>ind.setGenotype([0], ploidy, chrom)</tt>.
	 */
	void clearChromosome(const individual & ind, int ploidy, int chrom);

	/** Transmit chromosome \e chrom on the \e parPloidy set of homologous
	 *  chromosomes from \e parent to the \e ploidy set of homologous
	 *  chromosomes of \e offspring. It is equivalent to
	 *  <tt>offspring.setGenotype(parent.genotype(parPloidy, chrom), polidy, chrom)</tt>.
	 */
	void copyChromosome(const individual & parent, int parPloidy,
		individual & offspring, int ploidy, int chrom);

	/** Transmit the \e parPloidy set of homologous chromosomes from \e parent
	 *  to the \e ploidy set of homologous chromosomes of \e offspring.
	 *  Customized chromosomes are not copied. It is equivalent to
	 *  <tt>offspring.setGenotype(parent.genotype(parPloidy), ploidy)</tt>.
	 */
	void copyChromosomes(const individual & parent, int parPloidy,
		individual & offspring, int ploidy);

	/// HIDDEN
	string describe(bool format = true)
	{
		return "<simuPOP.GenoTransmitter>" ;
	}


	/** Initialize a base genotype operator for a population. This function should be
	 *  called before any other functions are used to transmit genotype.
	 */
	void initialize(const Population & pop);

	/// CPPONLY
	bool applyDuringMating(Population & pop,
	                       RawIndIterator offspring,
	                       individual * dad = NULL,
	                       individual * mom = NULL)
	{
		throw SystemError("The base genotype transmitter does not provide any function to transmit genotype");
	}


protected:
	// cache some genostructor information for
	// faster performance
	UINT m_ploidy;
	bool m_hasCustomizedChroms;
	vectoru m_lociToCopy;
	vectoru m_chromIdx;
};


/** This during mating operator copies parental genotype directly to offspring.
 *  This operator works for all mating schemes when one or two parents are
 *  involved. If both parents are passed, maternal genotype are copied. This
 *  genotype transmitter does not copy genotype on customized chromosomes.
 */
class CloneGenoTransmitter : public GenoTransmitter
{
public:
	/** Create a clone genotype transmitter (a during-mating operator) that
	 *  copies genotypes from parents to offspring. If two parents are
	 *  specified, genotypes are copied maternally. After genotype
	 *  transmission, offspring sex is copied from parental sex even if sex
	 *  has been determined by an offspring generator. All or specified
	 *  information fields (parameter \e infoFields, default to \c ALL_AVAIL)
	 *  will also be copied from parent to offspring. Parameters \e subPops
	 *  is ignored.
	 */
	CloneGenoTransmitter(const stringFunc & output = "", int begin = 0, int end = -1, int step = 1, const intList & at = vectori(),
		const intList & reps = intList(), const subPopList & subPops = subPopList(),
		const stringList & infoFields = stringList()) :
		GenoTransmitter(output, begin, end, step, at, reps, subPops, infoFields)
	{
	}


	/// Deep copy of a clone genotype transmitter.
	BaseOperator * clone() const
	{
		return new CloneGenoTransmitter(*this);
	}


	/// HIDDEN
	string describe(bool format = true);


	/// CPPONLY
	bool applyDuringMating(Population & pop,
		RawIndIterator offspring,
		individual * dad = NULL,
		individual * mom = NULL);

};


/** This Mendelian offspring generator accepts two parents and pass their
 *  genotypes to an offspring following Mendel's laws. Sex chromosomes are
 *  handled according to the sex of the offspring, which is usually determined
 *  in advance by an offspring generator. Customized chromosomes are not
 *  handled.
 */
class MendelianGenoTransmitter : public GenoTransmitter
{
public:
	/** Create a Mendelian genotype transmitter (a during-mating operator) that
	 *  transmits genotypes from parents to offspring following Mendel's laws.
	 *  Autosomes and sex chromosomes are handled but customized chromosomes
	 *  are ignored. Parameters \e subPops and \e infoFields are ignored.
	 */
	MendelianGenoTransmitter(const stringFunc & output = "", int begin = 0, int end = -1, int step = 1, const intList & at = vectori(),
		const intList & reps = intList(), const subPopList & subPops = subPopList(),
		const stringList & infoFields = vectorstr()) :
		GenoTransmitter(output, begin, end, step, at, reps, subPops, infoFields),
		m_chromX(-1), m_chromY(-1), m_numChrom(0)
	{
	}


	/// Deep copy of a Mendelian genotype transmitter.
	BaseOperator * clone() const
	{
		return new MendelianGenoTransmitter(*this);
	}


	/// HIDDEN
	string describe(bool format = true)
	{
		return "<simuPOP.MendelianGenoTransmitter>" ;
	}


	/// CPPONLY
	virtual bool applyDuringMating(Population & pop,
		RawIndIterator offspring,
		individual * dad = NULL,
		individual * mom = NULL);

	/** Initialize a base genotype operator for a population. This function should be
	 *  called before function \c transmitGenotype is used to transmit genotype.
	 */
	void initialize(const Population & pop);

	/** Transmit genotype from parent to offspring, and fill the \e ploidy
	 *  homologous set of chromosomes. This function does not set genotypes of
	 *  customized chromosomes and handles sex chromosomes properly, according
	 *  to offspring sex and \c ploidy.
	 */
	void transmitGenotype(const individual & parent,
		individual & offspring, int ploidy);

protected:
	// cache chromBegin, chromEnd for better performance.
	vectoru m_chIdx;

	int m_chromX;

	int m_chromY;

	UINT m_numChrom;
};


/** A genotype transmitter (during-mating operator) that transmits parental
 *  genotype of a parent through self-fertilization. That is to say, the
 *  offspring genotype is formed according to Mendel's laws, only that a
 *  parent serves as both maternal and paternal parents.
 */
class SelfingGenoTransmitter : public MendelianGenoTransmitter
{
public:
	/** Create a self-fertilization genotype transmitter that transmits
	 *  genotypes of a parent to an offspring through self-fertilization.
	 *  Cutsomized chromosomes are not handled. Parameters \e subPops and
	 *  \e infoFields are ignored.
	 */
	SelfingGenoTransmitter(const stringFunc & output = "", int begin = 0, int end = -1, int step = 1, const intList & at = vectori(),
		const intList & reps = intList(), const subPopList & subPops = subPopList(),
		const stringList & infoFields = vectorstr())
		: MendelianGenoTransmitter(output, begin, end, step, at, reps, subPops, infoFields)
	{
	}


	/// Deep copy of a selfing genotype transmitter
	BaseOperator * clone() const
	{
		return new SelfingGenoTransmitter(*this);
	}


	/// HIDDEN
	string describe(bool format = true)
	{
		return "<simuPOP.SelfingGenoTransmitter>" ;
	}


	/// CPPONLY
	bool applyDuringMating(Population & pop,
		RawIndIterator offspring,
		individual * dad = NULL,
		individual * mom = NULL);

};


/** A genotype transmitter (during-mating operator) for haplodiploid
 *  populations. The female parent is considered as diploid and the male parent
 *  is considered as haploid (only the first homologous copy is valid). If the
 *  offspring is \c FEMALE, she will get a random copy of two homologous
 *  chromosomes of her mother, and get the only paternal copy from her father.
 *  If the offspring is \c MALE, he will only get a set of chromosomes from his
 *  mother.
 */
class HaplodiploidGenoTransmitter : public MendelianGenoTransmitter
{
public:
	/** Create a haplodiploid genotype transmitter (during-mating operator)
	 *  that transmit parental genotypes from parents to offspring in a
	 *  haplodiploid population. Parameters \e subPops and \e infoFields
	 *  are ignored.
	 */
	HaplodiploidGenoTransmitter(const stringFunc & output = "", int begin = 0, int end = -1, int step = 1, const intList & at = vectori(),
		const intList & reps = intList(), const subPopList & subPops = subPopList(),
		const stringList & infoFields = vectorstr())
		: MendelianGenoTransmitter(output, begin, end, step, at, reps, subPops, infoFields),
		m_copier()
	{
	}


	/// Deep copy of a haplodiploid transmitter.
	BaseOperator * clone() const
	{
		return new HaplodiploidGenoTransmitter(*this);
	}


	/// HIDDEN
	string describe(bool format = true)
	{
		return "<simuPOP.HaplodiploidGenoTransmitter>" ;
	}


	/// HIDDEN
	void initialize(const Population & pop);

	/// CPPONLY
	virtual bool applyDuringMating(Population & pop,
		RawIndIterator offspring,
		individual * dad = NULL,
		individual * mom = NULL);

private:
	GenoTransmitter m_copier;
};


/** This geno transmitter assumes that the first homologous copy of several (or
 *  all) \c Customized chromosomes are copies of mitochondrial chromosomes. It
 *  transmits these chromosomes randomly from the female parent to offspring.
 *  If this transmitter is applied to populations with more than one homologous
 *  copies of chromosomes, it transmits the first homologous copy of
 *  chromosomes and clears alleles (set to zero) on other homologous copies.
 */
class MitochondrialGenoTransmitter : public GenoTransmitter
{
public:
	/** Createa a mitochondrial genotype transmitter that treats all Customized
	 *  chromosomes, or a list of chromosomes specified by \e chroms, as human
	 *  mitochondrial chromosomes. These chromosomes should have the same
	 *  length and the same number of loci. This operator transmits these
	 *  chromosomes randomly from the female parent to offspring of both sexes.
	 */
	MitochondrialGenoTransmitter(const stringFunc & output = "", const vectoru & chroms = vectoru(),
		int begin = 0, int end = -1, int step = 1, const intList & at = vectori(),
		const intList & reps = intList(), const subPopList & subPops = subPopList(),
		const stringList & infoFields = vectorstr())
		: GenoTransmitter(output, begin, end, step, at, reps, subPops, infoFields),
		m_chroms(chroms), m_mitoChroms(0), m_numLoci(0)
	{
	}


	/// Deep copy of a mitochondrial genotype transmitter.
	BaseOperator * clone() const
	{
		return new MitochondrialGenoTransmitter(*this);
	}


	/// HIDDEN
	string describe(bool format = true)
	{
		return "<simuPOP.MitochondrialGenoTransmitter>" ;
	}


	/// HIDDEN
	void initialize(const Population & pop);

	/// CPPONLY
	virtual bool applyDuringMating(Population & pop,
		RawIndIterator offspring,
		individual * dad = NULL,
		individual * mom = NULL);

private:
	// this is user input.
	vectoru m_chroms;

	// this is the temporary holder for different populaitons
	vectoru m_mitoChroms;

	//
	UINT m_numLoci;
};


/** A genotype transmitter (during-mating operator) that transmits parental
 *  chromosomes to offspring, subject to recombination and gene conversion.
 *  This can be used to replace \c MendelianGenoTransmitter and
 *  \c SelfingGenoTransmitter. It does not work in haplodiploid populations,
 *  although a customized genotype transmitter that makes uses this
 *  operator could be defined. Please refer to the simuPOP user's guide or
 *  online cookbook for details.
 *
 *  Recombination could be applied to all adjacent markers or after specified
 *  loci. Recombination rate between two adjacent markers could be specified
 *  directly, or calculated using physical distance between them. In the latter
 *  case, a recombination intensity is multiplied by physical distance between
 *  markers.
 *
 *  Gene conversion is interpreted as double-recombination events. That is to
 *  say, if a recombination event happens, it has a certain probability (can
 *  be 1) to become a conversion event, namely triggering another recombination
 *  event down the chromosome. The length of the converted chromosome can be
 *  controlled in a number of ways.
 *
 *  \note simuPOP does not assume any unit to loci positions so recombination
 *  intensity could be explained differntly (e.g. cM/Mb, Morgan/Mb) depending
 *  on your intepretation of loci positions. For example, if basepair is used
 *  for loci position, <tt>intensity=10^-8</tt> indicates <tt>10^-8</tt> per
 *  basepair, which is equivalent to <tt>10^-2</tt> per Mb or 1 cM/Mb. If \c Mb
 *  is used for physical positions, the same recombination intensity could be
 *  achieved by <tt>intensity=0.01</tt>.
 */
class Recombinator : public GenoTransmitter
{
public:
	/** Create a Recombinator (a mendelian genotype transmitter with
	 *  recombination and gene conversion) that passes genotypes from parents
	 *  (or a parent in case of self-fertilization) to offspring.
	 *
	 *  Recombination happens by default between all adjacent markers but can
	 *  be limited to a given set of \e loci. Each locus in this list specifies
	 *  a recombination point between the locus and the locus immediately
	 *  \b before it. Loci that are the first locus on each chromosome are
	 *  ignored.
	 *
	 *  If a single recombination rate (parameter \e rates) is specified, it
	 *  will used for all loci (all loci or loci specified by parameter
	 *  \e loci), regardless of physical distances between adjacent loci.
	 *
	 *  If a list of recombination rates are specified in \e rates, a parameter
	 *  \e loci with the same length should also be specified. Different
	 *  recombination rates can then be used after these loci (between
	 *  specified loci and their immediate neighbor to the right).
	 *
	 *  A recombination intensity (\e intensity) can be used to specify
	 *  recombination rates that are proportional to physical distances between
	 *  adjacent markers. If the physical distance between two markers is \c d,
	 *  the recombination rate between them will be <tt>intensity * d</tt>. No
	 *  unit is assume for loci position and recombination intensity.
	 *
	 *  Gene conversion is controlled using parameter \e convMode, which can be
	 *
	 *  \li <tt>NoConversion</tt>: no gene conversion (default).
	 *  \li <tt>(NUM_MARKERS, prob, n)</tt>: With probability \e prob, convert
	 *      a fixed number (\e n) of markers if a recombination event happens.
	 *  \li <tt>(GEOMETRIC_DISTRIBUTION, prob, p)</tt>: With probability \e prob,
	 *      convert a random number of markers if a recombination event happens.
	 *      The number of markes converted follows a geometric distribution
	 *      with probability \e p.
	 *  \li <tt>(TRACT_LENGTH, prob, n)</tt>: With probability \e prob, convert
	 *      a region of fixed tract length (\e n) if a recombination event
	 *      happens. The actual number of markers converted depends on loci
	 *      positions of surrounding loci. The starting position of this
	 *      tract is the middle of two adjacent markers. For example, if four
	 *      loci are located at <tt>0, 1, 2, 3</tt> respectively, a conversion
	 *      event happens between \c 0 and \c 1, with a tract length 2 will
	 *      start at 0.5 and end at 2.5, covering the second and third loci.
	 *  \li <tt>(EXPONENTIAL_DISTRIBUTION, prob, p)</tt>: With probability
	 *      \e prob, convert a region of random tract length if a recombination
	 *      event happens. The distribution of tract length follows a
	 *      exponential distribution with probability \c p. The actual number
	 *      of markers converted depends on loci positions of surrounding loci.
	 *
	 *  simuPOP uses this probabilistic model of gene conversion because when a
	 *  recombination event happens, it may become a recombination event if the
	 *  if the Holliday junction is resolved/repaired successfully, or a
	 *  conversion event if the junction is not resolved/repaired. The
	 *  probability, however, is more commonly denoted by the ratio of
	 *  conversion to recombination events in the literature. This ratio varies
	 *  greatly from study to study, ranging from 0.1 to 15 (Chen et al, Nature
	 *  Review Genetics, 2007). This translate to 0.1/0.9~0.1 to 15/16~0.94 of
	 *  the gene conversion probability.
	 *
	 *  A \c Recombinator usually does not send any output. However, if an
	 *  information field is given (parameter \e infoFields), this operator
	 *  will treat this information field as an unique ID of parents and
	 *  offspring and output all recombination events in the format of
	 *  <tt>offspring_id parent_id starting_ploidy loc1 loc2 ... </tt> where
	 *  \c starting_ploidy indicates which homologous copy genotype replication
	 *  starts from (\c 0 or \c 1), \c loc1, \c loc2 etc are loci after which
	 *  recombination events happens. If there are multiple chromosomes on the
	 *  genome, you will see a lot of (fake) recombination events because of
	 *  independent segregation of chromosomes. Such a record will be generated
	 *  for each set of homologous chromosomes so an diploid offspring will
	 *  have two lines of output. Note that individual IDs need to be set
	 *  (using a \c IdTagger operator) before this Recombinator is applied.
	 *
	 *  \note conversion tract length is usually short, and is estimated to be
	 *      between 337 and 456 bp, with overall range between maybe 50 - 2500
	 *      bp. This is usually not enough to convert, for example, two adjacent
	 *      markers from the HapMap dataset.
	 *
	 *  \note There is no recombination between sex chromosomes (Chromosomes X
	 *      and Y), although recombination is possible between pesudoautosomal
	 *      regions on these chromosomes. If such a feature is required, you
	 *      will have to simulate the pesudoautosomal regions as separate
	 *      chromosomes.
	 */
	Recombinator(const floatList & rates = vectorf(), double intensity = -1,
		const uintList & loci = uintList(), const floatList & convMode = NO_CONVERSION,
		const stringFunc & output = "", int begin = 0, int end = -1, int step = 1,
		const intList & at = vectori(),
		const intList & reps = intList(), const subPopList & subPops = subPopList(),
		const stringList & infoFields = vectorstr());


	/// deep copy of a Recombinator
	virtual BaseOperator * clone() const
	{
		return new Recombinator(*this);
	}


	virtual ~Recombinator()
	{
	}


	/// HIDDEN
	string describe(bool format = true);


	/** Initialize a Recombinator for the genotypic structure of population
	 *  \e pop. This function should be called before a Recombinator is
	 *  explicitly applied to a population.
	 */
	void initialize(const Population & pop);

	/** This function transmits genotypes from a \e parent to the \e ploidy-th
	 *  homologous set of chromosomes of an \e offspring. It can be used, for
	 *  example, by a customized genotype transmitter to use sex-specific
	 *  recombination rates to transmit parental genotypes to offspring.
	 */
	void transmitGenotype(const individual & parent,
		individual & offspring, int ploidy);

	/** CPPONLY
	 *  Apply the Recombinator during mating
	 */
	virtual bool applyDuringMating(Population & pop,
		RawIndIterator offspring,
		individual * dad, individual * mom);

private:
	/// determine number of markers to convert
	int markersConverted(size_t index, const individual & ind);

private:
	/// intensity
	double m_intensity;

	/// differnt rates
	vectorf m_rates;

	/// initial parameter
	uintList m_loci;

	/// position to recombine, changed to fit a special pop
	vectoru m_recBeforeLoci;

	vectorf m_convMode;

	/// bernulli trials
	//  vector<Bernullitrials*> m_bt;
	Bernullitrials m_bt;

	// locataion of special chromosomes
	int m_chromX;
	int m_chromY;
	int m_customizedBegin;
	int m_customizedEnd;

	/// algorithm to use (frequent or seldom recombinations)
	int m_algorithm;

	ostream * m_debugOutput;
};

}
#endif
