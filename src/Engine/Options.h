#ifndef OPTIONS_H
#define OPTIONS_H
#include "PsimagLite.h"
#include <cctype>
#include <algorithm>
#include <numeric>

namespace Dmrg {

template<typename InputValidatorType>
class Options {

public:

	typedef typename PsimagLite::String::value_type CharType;
	typedef typename PsimagLite::String::const_iterator StringConstIterator;
	typedef PsimagLite::Vector<PsimagLite::String>::Type VectorStringType;

	Options(PsimagLite::String label, InputValidatorType& io)
	{
		PsimagLite::String tmp;
		io.readline(tmp, label);
		PsimagLite::split(vdata_, tmp, ",");
		std::transform(vdata_.begin(),
		               vdata_.end(),
		               vdata_.begin(),
		               [](PsimagLite::String s){ return toLower(s); });
	}

	void operator+=(PsimagLite::String moreData)
	{
		VectorStringType vmore;
		PsimagLite::split(vmore, moreData, ",");
		vdata_.insert(vdata_.end(), vmore.begin(), vmore.end());
	}

	void write(PsimagLite::String label, PsimagLite::IoSerializer& ioSerializer) const
	{
		const PsimagLite::String tmp = std::accumulate(vdata_.begin(),
		                                               vdata_.end(),
		                                               PsimagLite::String(","));
		ioSerializer.write(label, tmp);
	}

	bool isSet(PsimagLite::String what) const
	{
		what = toLower(what);
		VectorStringType::const_iterator it = std::find(vdata_.begin(),
		                                                vdata_.end(),
		                                                what);
		return (it != vdata_.end());
	}

private:

	static PsimagLite::String toLower(PsimagLite::String data)
	{
		std::transform(data.begin(), data.end(), data.begin(),
		               [](unsigned char c){ return std::tolower(c); });
		return data;
	}

	VectorStringType vdata_;
};
}
#endif // OPTIONS_H
