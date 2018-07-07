/*
Copyright (c) 2009-2012-2018, UT-Battelle, LLC
All rights reserved

[DMRG++, Version 5.]
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
/** \ingroup DMRG */
/*@{*/

/*! \file ModelCommon.h
 *
 *  An abstract class to represent the strongly-correlated-electron models that
 *  can be used with the DmrgSolver
 *
 */

#ifndef MODEL_COMMON_H
#define MODEL_COMMON_H
#include <iostream>

#include "VerySparseMatrix.h"
#include "HamiltonianConnection.h"
#include "Su2SymmetryGlobals.h"
#include "InputNg.h"
#include "InputCheck.h"
#include "ProgressIndicator.h"
#include "NoPthreads.h"
#include "Sort.h"
#include "Profiling.h"

namespace Dmrg {


template<typename ModelBaseType,typename LinkProductType>
class ModelCommon : public ModelBaseType::ModelCommonBaseType {

	typedef typename ModelBaseType::ModelCommonBaseType ModelCommonBaseType;
	typedef typename ModelBaseType::ModelHelperType ModelHelperType;
	typedef typename ModelBaseType::GeometryType GeometryType;
	typedef typename ModelHelperType::SparseMatrixType SparseMatrixType;
	typedef typename SparseMatrixType::value_type SparseElementType;
	typedef VerySparseMatrix<SparseElementType> VerySparseMatrixType;
	typedef typename ModelHelperType::LinkType LinkType;
	typedef typename GeometryType::AdditionalDataType AdditionalDataType;

public:

	typedef PsimagLite::InputNg<InputCheck>::Readable InputValidatorType;
	typedef typename ModelHelperType::OperatorsType OperatorsType;
	typedef typename ModelHelperType::BlockType Block;
	typedef typename ModelHelperType::RealType RealType;
	typedef typename ModelHelperType::BasisType MyBasis;
	typedef typename ModelHelperType::BasisWithOperatorsType BasisWithOperatorsType;
	typedef HamiltonianConnection<GeometryType,ModelHelperType,LinkProductType>
	HamiltonianConnectionType;
	typedef typename HamiltonianConnectionType::LinkProductStructType LinkProductStructType;
	typedef typename ModelHelperType::LeftRightSuperType LeftRightSuperType;
	typedef typename OperatorsType::OperatorType OperatorType;
	typedef typename PsimagLite::Vector<OperatorType>::Type VectorOperatorType;
	typedef typename ModelBaseType::SolverParamsType SolverParamsType;
	typedef typename PsimagLite::Vector<LinkProductStructType>::Type VectorLinkProductStructType;
	typedef typename HamiltonianConnectionType::VectorSizeType VectorSizeType;
	typedef typename HamiltonianConnectionType::HamiltonianAbstractType HamiltonianAbstractType;

	ModelCommon(const SolverParamsType& params,const GeometryType& geometry)
	    : ModelCommonBaseType(params,geometry),
	      progress_("ModelCommon")
	{
		if (LinkProductType::terms() > this->geometry().terms()) {
			PsimagLite::String str("ModelCommon: NumberOfTerms must be ");
			str += ttos(LinkProductType::terms()) + " in input file for this model\n";
			throw PsimagLite::RuntimeError(str);
		}

		Su2SymmetryGlobals<RealType>::init(ModelHelperType::isSu2());
		MyBasis::useSu2Symmetry(ModelHelperType::isSu2());
	}

	/** Let H be the hamiltonian of the  model for basis1 and partition m
	 * consisting of the external product
		 * of basis2 \otimes basis3
		 * This function does x += H*y
		 * The \cppFunction{matrixVectorProduct} function implements the operation $x+=Hy$.
		 * This function
		 * has a default implementation.
		 */
	void matrixVectorProduct(typename PsimagLite::Vector<SparseElementType>::Type& x,
	                         const typename PsimagLite::Vector<SparseElementType>::Type& y,
	                         ModelHelperType const &modelHelper) const
	{
		//! contribution to Hamiltonian from connection system-environment
		hamiltonianConnectionProduct(x,y,modelHelper);

		// parts below were move to include them in the parallelization
		//! contribution to Hamiltonian from current system moved to HamiltonianConnection
		// modelHelper.hamiltonianLeftProduct(x,y);
		//! contribution to Hamiltonian from current environ moved to HamiltonianConnection
		// modelHelper.hamiltonianRightProduct(x,y);
	}

	/**
		The function \cppFunction{addHamiltonianConnection} implements
		the Hamiltonian connection (e.g. tight-binding links in the case of the Hubbard Model
		or products $S_i\cdot S_j$ in the case of the Heisenberg model) between
		two basis, $basis2$ and $basis3$, in the order of the outer product,
		$basis1={\rm SymmetryOrdering}(basis2\otimes basis3)$. This was
		explained before in Section~\ref{subsec:dmrgBasisWithOperators}.
		This function has a default implementation.
		*/
	void addHamiltonianConnection(SparseMatrixType &matrix,
	                              const LeftRightSuperType& lrs,
	                              RealType currentTime) const
	{
		PsimagLite::Profiling profiling("addHamiltonianConnection", std::cout);
		assert(lrs.super().partition() > 0);
		SizeType total = lrs.super().partition()-1;

		typename PsimagLite::Vector<VerySparseMatrixType*>::Type vvsm(total, 0);
		VectorSizeType nzs(total, 0);

		for (SizeType m = 0; m < total; ++m) {
			SizeType offset = lrs.super().partition(m);
			assert(lrs.super().partition(m + 1) >= offset);
			SizeType bs = lrs.super().partition(m + 1) - offset;

			vvsm[m] = new VerySparseMatrixType(bs, bs);
			VerySparseMatrixType& vsm = *(vvsm[m]);
			SizeType threadId = 0;
			ModelHelperType modelHelper(m, lrs, currentTime, threadId);
			addHamiltonianConnection(vsm, modelHelper);
			nzs[m] = vsm.nonZeros();
			if (nzs[m] > 0) continue;
			delete vvsm[m];
			vvsm[m] = 0;
		}

		PsimagLite::Sort<VectorSizeType> sort;
		VectorSizeType permutation(total, 0);
		sort.sort(nzs, permutation);

		typename PsimagLite::Vector<const SparseMatrixType*>::Type vectorOfCrs;

		assert(total == permutation.size());
		for (SizeType i = 0; i < total; ++i) { // loop over new order

			SizeType m = permutation[i]; // get old index from new index

			if (vvsm[m] == 0) continue;

			const VerySparseMatrixType& vsm = *(vvsm[m]);
			SparseMatrixType matrixBlock2;
			matrixBlock2 = vsm;
			delete vvsm[m];
			vvsm[m] = 0;

			SizeType offset = lrs.super().partition(m);
			SparseMatrixType* full = new SparseMatrixType(matrix.rows(),
			                                              matrix.cols(),
			                                              matrixBlock2.nonZeros());
			fromBlockToFull(*full, matrixBlock2, offset);
			vectorOfCrs.push_back(full);
		}

		if (vectorOfCrs.size() == 0) return;

		vectorOfCrs.push_back(&matrix);
		SizeType effectiveTotal = vectorOfCrs.size();

		typename PsimagLite::Vector<SparseElementType>::Type ones(effectiveTotal, 1.0);
		SparseMatrixType sumCrs;
		sum(sumCrs, vectorOfCrs, ones);
		vectorOfCrs.pop_back();
		effectiveTotal = vectorOfCrs.size();
		for (SizeType i = 0; i < effectiveTotal; ++i) {
			delete vectorOfCrs[i];
			vectorOfCrs[i] = 0;
		}

		matrix.swap(sumCrs);
	}

private:

	/**
		Let $H_m$ be the Hamiltonian connection between basis2 and basis3 in
		the orderof basis1 for block $m$. Then this function does $x+= H_m *y$
		*/
	void hamiltonianConnectionProduct(typename PsimagLite::Vector<SparseElementType>::Type& x,
	                                  const typename PsimagLite::Vector<SparseElementType>::Type& y,
	                                  const ModelHelperType& modelHelper) const
	{
		const LinkProductStructType& lpsConst = modelHelper.lps();
		LinkProductStructType& lps = const_cast<LinkProductStructType&>(lpsConst);
		LinkProductStructType lpsOne(ProgramGlobals::MAX_LPS);
		HamiltonianConnectionType hc(this->geometry(),modelHelper,&lps,&x,&y);

		HamiltonianAbstractType hamiltonianAbstract(modelHelper.leftRightSuper().super().block());

		SizeType total = 0;
		SizeType nitems = hamiltonianAbstract.items();
		for (SizeType x = 0; x < nitems; ++x) {
			SizeType totalOne = 0;
			hc.compute(hamiltonianAbstract, x, 0, &lpsOne, totalOne);
			if (!lps.sealed)
				lps.push(lpsOne,totalOne);
			else
				lps.copy(lpsOne,totalOne,total);
			total += totalOne;
		}

		hc.tasks(total + 2);
		assert(lps.typesaved.size() == total);
		if (!lps.sealed) {
			PsimagLite::OstringStream msg;
			msg<<"LinkProductStructSize="<<total;
			progress_.printline(msg,std::cout);
			lps.sealed = true;

			PsimagLite::OstringStream msg2;
			// add left and right contributions
			msg2<<"PthreadsTheoreticalLimitForThisPart="<<(total+2);

			// The theoretical maximum number of pthreads that are useful
			// is equal to C + 2, where
			// C = number of connection = 2*G*M
			// where G = geometry factor
			// and   M = model factor
			// G = 1 for a chain
			// G = leg for a ladder with leg legs. (For instance, G=2 for a 2-leg ladder).
			// M = 2 for the Hubbard model
			//
			// In general, M = \sum_{term=0}^{terms} dof(term)
			// where terms and dof(term) is model dependent.
			// To find M for a model, go to the Model directory and see LinkProduct*.h file.
			// the return of function terms() for terms.
			// For dof(term) see function dof(SizeType term,...).
			// For example, for the HubbardModelOneBand, one must look at
			// src/Model/HubbardOneBand/LinkProductHubbardOneBand.h
			// In that file, terms() returns 1, so that terms=1
			// Therefore there is only one term: term = 0.
			// And dof(0,...) = 2, as you can see in  LinkProductHubbardOneBand.h.
			// Then M = 2.
			progress_.printline(msg2,std::cout);
		}

		typedef PsimagLite::Parallelizer<HamiltonianConnectionType> ParallelizerType;
		ParallelizerType parallelConnections(PsimagLite::Concurrency::codeSectionParams);
		parallelConnections.loopCreate(hc);

		hc.sync();
	}

	SizeType getLinkProductStruct(const ModelHelperType& modelHelper) const
	{
		typename PsimagLite::Vector<SparseElementType>::Type x,y; // bogus

		const LinkProductStructType& lpsConst = modelHelper.lps();
		LinkProductStructType& lps = const_cast<LinkProductStructType&>(lpsConst);
		LinkProductStructType lpsOne(ProgramGlobals::MAX_LPS);
		HamiltonianConnectionType hc(this->geometry(),modelHelper,&lps,&x,&y);

		HamiltonianAbstractType hamiltonianAbstract(modelHelper.leftRightSuper().super().block());

		SizeType total = 0;
		SizeType nitems = hamiltonianAbstract.items();
		for (SizeType x = 0; x < nitems; ++x) {
			SizeType totalOne = 0;
			hc.compute(hamiltonianAbstract, x, 0, &lpsOne, totalOne);
			if (!lps.sealed)
				lps.push(lpsOne,totalOne);
			else
				lps.copy(lpsOne,totalOne,total);
			total += totalOne;
		}

		if (lps.typesaved.size() != total) {
			PsimagLite::String str("getLinkProductStruct: InternalError\n");
			throw PsimagLite::RuntimeError(str);
		}

		if (!lps.sealed) {
			PsimagLite::OstringStream msg;
			msg<<"LinkProductStructSize="<<total;
			progress_.printline(msg,std::cout);
			lps.sealed = true;
		}

		return total;
	}

	LinkType getConnection(const SparseMatrixType** A,
	                       const SparseMatrixType** B,
	                       SizeType ix,
	                       const ModelHelperType& modelHelper) const
	{
		const LinkProductStructType& lps = modelHelper.lps();
		typename PsimagLite::Vector<SparseElementType>::Type x,y; // bogus
		HamiltonianConnectionType hc(this->geometry(),modelHelper,&lps,&x,&y);
		SizeType xx = 0;
		ProgramGlobals::ConnectionEnum type;
		SizeType term = 0;
		SizeType dofs = 0;
		SparseElementType tmp = 0.0;
		AdditionalDataType additionalData;
		hc.prepare(ix,xx,type,tmp,term,dofs,additionalData);
		LinkType link2 = hc.getKron(A,B,i,j,type,tmp,term,dofs,additionalData);
		return link2;
	}

	/**
		Returns H, the hamiltonian for basis1 and partition
		$m$ consisting of the external product of basis2$\otimes$basis3
		Note: Used only for debugging purposes
		*/
	void fullHamiltonian(SparseMatrixType& matrix,const ModelHelperType& modelHelper) const
	{
		SparseMatrixType matrixBlock;

		//! contribution to Hamiltonian from current system
		modelHelper.calcHamiltonianPart(matrixBlock,true);
		matrix = matrixBlock;

		//! contribution to Hamiltonian from current envirnoment
		modelHelper.calcHamiltonianPart(matrixBlock,false);
		matrix += matrixBlock;

		matrixBlock.clear();

		VerySparseMatrixType vsm(matrix);
		addHamiltonianConnection(vsm,modelHelper);

		matrix = vsm;
	}

	void addConnectionsInNaturalBasis(SparseMatrixType& hmatrix,
	                                  const VectorOperatorType& cm,
	                                  const Block& block,
	                                  bool sysEnvOnly,
	                                  RealType time) const
	{
		if (block.size() != 2)
			err("addConnectionsInNaturalBasis(): unimplemented\n");
	}

	// Add Hamiltonian connection between basis2 and basis3
	// in the orderof basis1 for symmetry block m
	void addHamiltonianConnection(VerySparseMatrix<SparseElementType>& matrix,
	                              const ModelHelperType& modelHelper) const
	{
		SizeType matrixRank = matrix.rows();
		VerySparseMatrixType matrix2(matrixRank, matrixRank);
		typedef HamiltonianConnection<
		        GeometryType,
		        ModelHelperType,
		        LinkProductType> SomeHamiltonianConnectionType;
		SomeHamiltonianConnectionType hc(this->geometry(),modelHelper);

		SizeType total = 0;
		SizeType nitems = hc.items();
		for (SizeType x = 0; x < nitems; ++x) {
			SparseMatrixType matrixBlock(matrixRank, matrixRank);
			if (!hc.compute(x, &matrixBlock, 0, total))
				continue;
			VerySparseMatrixType vsm(matrixBlock);
			matrix2+=vsm;
		}

		matrix += matrix2;
	}

	SparseMatrixType transposeOrNot(const SparseMatrixType& A,char mod) const
	{
		if (mod == 'C' || mod == 'c') {
			SparseMatrixType Ac;
			transposeConjugate(Ac,A);
			return Ac;
		}
		return A;
	}

	PsimagLite::ProgressIndicator progress_;
};     //class ModelCommon
} // namespace Dmrg
/*@}*/
#endif
