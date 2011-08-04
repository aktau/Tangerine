
#ifndef MERGEACTIONTYPES_H_
#define MERGEACTIONTYPES_H_

namespace Merge {
	typedef enum {
		NONE,
		DONT_MERGE, // this is NOT the same as NONE: NONE is an invalid action, DONT_MERGE is not
		//CHOOSE_MASTER, // this is in fact the same as DONT_MERGE, disabling until I find a valid use
		CHOOSE_SLAVE,
		CHOOSE_HISTORY,
		PREFER_USER,
		MOST_RECENT,
		ASSIGN_NEW_ID
	} Action;
}



#endif /* MERGEACTIONTYPES_H_ */
