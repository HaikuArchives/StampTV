/*

	Benaphore.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#include "Benaphore.h"

//------------------------------------------------------------------------------
status_t Benaphore::Lock(const uint32 a_flags, const bigtime_t a_timeout)
{
	if (atomic_add (&fCount, 1) > 0)
	{
		status_t err (B_OK);
		while ((err = acquire_sem_etc (fSem, 1, a_flags, a_timeout)) == B_INTERRUPTED)
		{
			;
		}

		if (err != B_OK && err != B_BAD_SEM_ID)
		{
			atomic_add (&fCount, -1);
			if (err != B_WOULD_BLOCK && err != B_TIMED_OUT)
				err = B_BAD_SEM_ID;
		}
		return err;
	}
	return B_OK;	
}
