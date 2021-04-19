#ifndef DISKORMEMORYSTACK_H
#define DISKORMEMORYSTACK_H
#include "Stack.h"
#include "DiskStackNg.h"
#include "Io/IoNg.h"

namespace Dmrg {

template<typename BasisWithOperatorsType>
class DiskOrMemoryStack {

public:

	typedef typename PsimagLite::Stack<BasisWithOperatorsType>::Type MemoryStackType;
	typedef DiskStack<BasisWithOperatorsType> DiskStackType;

	DiskOrMemoryStack(bool onDisk,
	                  const PsimagLite::String filename,
	                  const PsimagLite::String post,
	                  PsimagLite::String label,
	                  bool isObserveCode)
	    : isObserveCode_(isObserveCode), diskW_(0), diskR_(0)
	{
		if (!onDisk) return;

		size_t lastindex = filename.find_last_of(".");
		PsimagLite::String file = filename.substr(0, lastindex) + post + ".hd5";

		if (createFile_) {
			PsimagLite::IoNg::Out out(file, PsimagLite::IoNg::ACC_TRUNC);
			out.close();
			createFile_ = false;
		}

		diskW_ = new DiskStackType(file, false, label, isObserveCode);
		diskR_ = new DiskStackType(file, true, label, isObserveCode);
	}

	~DiskOrMemoryStack()
	{
		delete diskR_;
		diskR_ = 0;
		delete diskW_;
		diskW_ = 0;
	}

	void push(const BasisWithOperatorsType& b)
	{
		if (diskW_) {
			diskW_->push(b);
			diskW_->flush();
			diskR_->restore(diskW_->size());
		} else {
			memory_.push(b);
		}
	}

	void pop()
	{
		if (diskW_) {
			diskW_->pop();
			diskW_->flush();
			diskR_->restore(diskW_->size());
		} else {
			memory_.pop();
		}
	}

	bool onDisk() const { return (diskR_); }

	SizeType size() const
	{
		return (diskR_) ? diskR_->size() : memory_.size();
	}

	const BasisWithOperatorsType& top() const
	{
		return (diskR_) ? diskR_->top() : memory_.top();
	}

	void toDisk(DiskStackType& disk) const
	{
		if (diskR_) {
			SizeType total = diskR_->size();
			DiskStackType& diskNonConst = const_cast<DiskStackType&>(*diskR_);
			loadStack(disk, diskNonConst);
			assert(diskW_);
			diskW_->restore(total);
		} else {
			MemoryStackType memory = memory_;
			loadStack(disk, memory);
		}
	}

	void read(PsimagLite::String prefix, PsimagLite::IoNgSerializer& io)
	{
		if (diskW_) {
			assert(diskR_);
			err("read\n");
		}

		io.read(memory_, prefix);
	}

	void write(PsimagLite::String prefix, PsimagLite::IoNgSerializer& io) const
	{
		if (diskW_) {
			assert(diskR_);
			std::cerr<<__FILE__<<": write(): not writing, cannot restart\n";
			std::cout<<__FILE__<<": write(): not writing, cannot restart\n";
			return;
		}

		MemoryStackType m = memory_;
		io.write(prefix, m);
	}

	template<typename StackType1,typename StackType2>
	static void loadStack(StackType1& stackInMemory, StackType2& stackInDisk)
	{
		while (stackInDisk.size()>0) {
			BasisWithOperatorsType b = stackInDisk.top();
			stackInMemory.push(b);
			stackInDisk.pop();
		}
	}

	static bool createFile_;

private:

	DiskOrMemoryStack(const DiskOrMemoryStack&);

	DiskOrMemoryStack& operator=(const DiskOrMemoryStack&);

	bool isObserveCode_;
	mutable MemoryStackType memory_;
	DiskStackType *diskW_;
	DiskStackType *diskR_;
};

template<typename BasisWithOperatorsType>
bool DiskOrMemoryStack<BasisWithOperatorsType>::createFile_ = true;
}
#endif // DISKORMEMORYSTACK_H
