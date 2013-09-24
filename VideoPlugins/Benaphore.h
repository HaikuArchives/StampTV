/*

	Benaphore.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#ifndef _BENAPHORE_H_
#define _BENAPHORE_H_

#include <OS.h>

class Benaphore
{
	public:
		Benaphore (const char a_name[B_OS_NAME_LENGTH] = "Benaphore") :
			fSem (create_sem (0, a_name)), fCount (0)  { }
		~Benaphore ()
		{ delete_sem (fSem);	}

		status_t Lock (const uint32 a_flags = 0, const bigtime_t a_timeout = B_INFINITE_TIMEOUT);

		void Unlock (const uint32 a_flags = 0)
		{
			if (atomic_add (&fCount, -1) > 1)
				release_sem_etc (fSem, 1, a_flags);
		}
	
	private:
		sem_id	fSem;
		int32	fCount;
};

#endif // _BENAPHORE_H_
