#ifndef MERGECONFLICT_H_
#define MERGECONFLICT_H_

#include <QString>

class MergeConflict {
	public:
		MergeConflict();
		virtual ~MergeConflict();

	private:
		QString message;
};

#endif /* MERGECONFLICT_H_ */
