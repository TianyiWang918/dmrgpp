// BEGIN LICENSE BLOCK
/*
Copyright (c) 2009, UT-Battelle, LLC
All rights reserved

[DMRG++, Version 2.0.0]
[by G.A., Oak Ridge National Laboratory]

UT Battelle Open Source Software License 11242008

OPEN SOURCE LICENSE

Subject to the conditions of this License, each
contributor to this software hereby grants, free of
charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), a
perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable copyright license to use, copy,
modify, merge, publish, distribute, and/or sublicense
copies of the Software.

1. Redistributions of Software must retain the above
copyright and license notices, this list of conditions,
and the following disclaimer.  Changes or modifications
to, or derivative works of, the Software should be noted
with comments and the contributor and organization's
name.

2. Neither the names of UT-Battelle, LLC or the
Department of Energy nor the names of the Software
contributors may be used to endorse or promote products
derived from this software without specific prior written
permission of UT-Battelle.

3. The software and the end-user documentation included
with the redistribution, with or without modification,
must include the following acknowledgment:

"This product includes software produced by UT-Battelle,
LLC under Contract No. DE-AC05-00OR22725  with the
Department of Energy."
 
*********************************************************
DISCLAIMER

THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER, CONTRIBUTORS, UNITED STATES GOVERNMENT,
OR THE UNITED STATES DEPARTMENT OF ENERGY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED
STATES DEPARTMENT OF ENERGY, NOR THE COPYRIGHT OWNER, NOR
ANY OF THEIR EMPLOYEES, REPRESENTS THAT THE USE OF ANY
INFORMATION, DATA, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.

*********************************************************


*/
// END LICENSE BLOCK
/** \ingroup DMRG */
/*@{*/

/*! \file MettsCollapse.h
 *
 *  Stochastics (random choices) needed for the METTS algorithm 
 *
 */
 
#ifndef METTS_COLLAPSE_H
#define METTS_COLLAPSE_H
#include <iostream>
#include <vector>
#include "ProgressIndicator.h"
#include "PackIndices.h"

namespace Dmrg {
	template<typename VectorWithOffsetType,typename MettsStochasticsType>
	class MettsCollapse  {
		typedef typename VectorWithOffsetType::VectorType VectorType;
		typedef typename MettsStochasticsType::PairType PairType;
		typedef typename MettsStochasticsType::LeftRightSuperType LeftRightSuperType;
		
	public:
		typedef PsimagLite::PackIndices PackIndicesType;
		
		MettsCollapse(const MettsStochasticsType& mettsStochastics,
		              const LeftRightSuperType& lrs)
		: mettsStochastics_(mettsStochastics),lrs_(lrs)
		{}

		void operator()(VectorWithOffsetType& dest2,
		                const VectorWithOffsetType& src2,
		                const PairType& sites)
		{
			if (dest2.size()==0) {
				dest2 =  src2;
			}
			VectorWithOffsetType dest = src2;
			size_t alphaFixed = mettsStochastics_.chooseRandomState(sites.first);
			size_t betaFixed = mettsStochastics_.chooseRandomState(sites.second);
			collapseVector(dest,dest2,alphaFixed,betaFixed);
			assert(std::norm(dest)>1e-6);
			dest2 = dest;
		}

	private:
		void collapseVector(VectorWithOffsetType& dest,
							const VectorWithOffsetType& src,
							size_t alphaFixed,
							size_t betaFixed)
		{
			assert(src.sectors()==1);
			
			for (size_t ii=0;ii<src.sectors();ii++) {
				size_t i0 = src.sector(ii);
				VectorType vdest,vsrc;
				src.extract(vsrc,i0);
				collapseVector(vdest,vsrc,i0,alphaFixed,betaFixed);
				dest.setDataInSector(vdest,i0);
			}
			assert(std::norm(dest)>1e-6);
		}

		void collapseVector(VectorType& w,
							const VectorType& v,
							size_t m,
							size_t alphaFixed,
							size_t betaFixed)
		{
			int offset = lrs_.super().partition(m);
			int total = lrs_.super().partition(m+1) - offset;
			
			size_t nk = mettsStochastics_.hilbertSizePerSite();
			size_t ns = lrs_.left().size();
			PackIndicesType packSuper(ns);
			PackIndicesType packLeft(ns/nk);
			PackIndicesType packRight(nk);
			w.resize(total);
			for (size_t i=0;i<size_t(total);i++) {
				w[i] = 0;
				size_t alpha,beta;
				packSuper.unpack(alpha,beta,lrs_.super().permutation(i+offset));
				size_t alpha0,alpha1;
				packLeft.unpack(alpha0,alpha1,alpha);
				size_t beta0,beta1;
				packRight.unpack(beta0,beta1,beta);
				if (alpha1!=alphaFixed || beta0 != betaFixed) continue;
				w[i] = v[i];
			}
		}

		const MettsStochasticsType& mettsStochastics_;
		const LeftRightSuperType& lrs_;
	};  //class MettsCollapse
} // namespace Dmrg
/*@}*/
#endif //METTS_COLLAPSE_H
