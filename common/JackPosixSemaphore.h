/*
Copyright (C) 2004-2006 Grame  

This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __JackPosixSemaphore__
#define __JackPosixSemaphore__

#include "JackSynchro.h"
#include <semaphore.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

namespace Jack
{

/*!
\brief Inter process synchronization using POSIX semaphore.
*/

class JackPosixSemaphore : public JackSynchro
{

    private:

        sem_t* fSemaphore;

    protected:

        void BuildName(const char* name, const char* server_name, char* res);

    public:

        JackPosixSemaphore(): JackSynchro(), fSemaphore(NULL)
        {}
        virtual ~JackPosixSemaphore()
        {}

        bool Signal();
        bool SignalAll();
        bool Wait();
        bool TimedWait(long usec);

        bool Allocate(const char* name, const char* server_name, int value);
        bool Connect(const char* name, const char* server_name);
        bool ConnectInput(const char* name, const char* server_name);
        bool ConnectOutput(const char* name, const char* server_name);
        bool Disconnect();
        void Destroy();
};

} // end of namespace


#endif

