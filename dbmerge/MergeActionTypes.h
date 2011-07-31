
#ifndef MERGEACTIONTYPES_H_
#define MERGEACTIONTYPES_H_

namespace Merge {
	typedef enum {
		NONE,
		CHOOSE_MASTER,
		CHOOSE_SLAVE,
		PREFER_USER,
		MOST_RECENT,
		DONT_MERGE,
		ASSIGN_NEW_ID
	} Action;
}



#endif /* MERGEACTIONTYPES_H_ */
