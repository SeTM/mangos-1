/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "Threading.h"
#include "Utilities/UnorderedMapSet.h"
#include "Database/SqlDelayThread.h"
#include <ace/Recursive_Thread_Mutex.h>
#include "Policies/ThreadingModel.h"
#include <ace/TSS_T.h>

class SqlTransaction;
class SqlResultQueue;
class SqlQueryHolder;

#define MAX_QUERY_LEN   32*1024

//
class MANGOS_DLL_SPEC SqlConnection : public MaNGOS::ObjectLevelLockable<SqlConnection, ACE_Recursive_Thread_Mutex>
{
    public:
        virtual ~SqlConnection() {}

        //method for initializing DB connection
        virtual bool Initialize(const char *infoString) = 0;
        //public methods for making queries
        virtual QueryResult* Query(const char *sql) = 0;
        virtual QueryNamedResult* QueryNamed(const char *sql) = 0;

        //public methods for making requests
        virtual bool Execute(const char *sql) = 0;

        //escape string generation
        virtual unsigned long escape_string(char *to, const char *from, unsigned long length) { strncpy(to,from,length); return length; }

        // nothing do if DB not support transactions
        virtual bool BeginTransaction() { return true; }
        virtual bool CommitTransaction() { return true; }
        // can't rollback without transaction support
        virtual bool RollbackTransaction() { return true; }
};

class MANGOS_DLL_SPEC Database
{
    public:
        virtual ~Database();

        virtual bool Initialize(const char *infoString);
        virtual void InitDelayThread();
        virtual void HaltDelayThread();

        /// Synchronous DB queries
        inline QueryResult* Query(const char *sql)
        {
            SqlConnection::Lock guard(*m_pQueryConn);
            return m_pQueryConn->Query(sql);
        }

        inline QueryNamedResult* QueryNamed(const char *sql)
        {
            SqlConnection::Lock guard(*m_pQueryConn);
            return m_pQueryConn->QueryNamed(sql);
        }

        QueryResult* PQuery(const char *format,...) ATTR_PRINTF(2,3);
        QueryNamedResult* PQueryNamed(const char *format,...) ATTR_PRINTF(2,3);

        bool DirectExecute(const char* sql);
        bool DirectPExecute(const char *format,...) ATTR_PRINTF(2,3);

        /// Async queries and query holders, implemented in DatabaseImpl.h

        // Query / member
        template<class Class>
            bool AsyncQuery(Class *object, void (Class::*method)(QueryResult*), const char *sql);
        template<class Class, typename ParamType1>
            bool AsyncQuery(Class *object, void (Class::*method)(QueryResult*, ParamType1), ParamType1 param1, const char *sql);
        template<class Class, typename ParamType1, typename ParamType2>
            bool AsyncQuery(Class *object, void (Class::*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char *sql);
        template<class Class, typename ParamType1, typename ParamType2, typename ParamType3>
            bool AsyncQuery(Class *object, void (Class::*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char *sql);
        // Query / static
        template<typename ParamType1>
            bool AsyncQuery(void (*method)(QueryResult*, ParamType1), ParamType1 param1, const char *sql);
        template<typename ParamType1, typename ParamType2>
            bool AsyncQuery(void (*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char *sql);
        template<typename ParamType1, typename ParamType2, typename ParamType3>
            bool AsyncQuery(void (*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char *sql);
        // PQuery / member
        template<class Class>
            bool AsyncPQuery(Class *object, void (Class::*method)(QueryResult*), const char *format,...) ATTR_PRINTF(4,5);
        template<class Class, typename ParamType1>
            bool AsyncPQuery(Class *object, void (Class::*method)(QueryResult*, ParamType1), ParamType1 param1, const char *format,...) ATTR_PRINTF(5,6);
        template<class Class, typename ParamType1, typename ParamType2>
            bool AsyncPQuery(Class *object, void (Class::*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char *format,...) ATTR_PRINTF(6,7);
        template<class Class, typename ParamType1, typename ParamType2, typename ParamType3>
            bool AsyncPQuery(Class *object, void (Class::*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char *format,...) ATTR_PRINTF(7,8);
        // PQuery / static
        template<typename ParamType1>
            bool AsyncPQuery(void (*method)(QueryResult*, ParamType1), ParamType1 param1, const char *format,...) ATTR_PRINTF(4,5);
        template<typename ParamType1, typename ParamType2>
            bool AsyncPQuery(void (*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char *format,...) ATTR_PRINTF(5,6);
        template<typename ParamType1, typename ParamType2, typename ParamType3>
            bool AsyncPQuery(void (*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char *format,...) ATTR_PRINTF(6,7);
        template<class Class>
        // QueryHolder
            bool DelayQueryHolder(Class *object, void (Class::*method)(QueryResult*, SqlQueryHolder*), SqlQueryHolder *holder);
        template<class Class, typename ParamType1>
            bool DelayQueryHolder(Class *object, void (Class::*method)(QueryResult*, SqlQueryHolder*, ParamType1), SqlQueryHolder *holder, ParamType1 param1);

        bool Execute(const char *sql);
        bool PExecute(const char *format,...) ATTR_PRINTF(2,3);

        // Writes SQL commands to a LOG file (see mangosd.conf "LogSQL")
        bool PExecuteLog(const char *format,...) ATTR_PRINTF(2,3);

        bool BeginTransaction();
        bool CommitTransaction();
        bool RollbackTransaction();
        //for sync transaction execution
        bool CommitTransactionDirect();

        operator bool () const { return m_pQueryConn != 0 && m_pAsyncConn != 0; }

        //escape string generation
        void escape_string(std::string& str);

        // must be called before first query in thread (one time for thread using one from existing Database objects)
        virtual void ThreadStart();
        // must be called before finish thread run (one time for thread using one from existing Database objects)
        virtual void ThreadEnd();

        // set database-wide result queue. also we should use object-bases and not thread-based result queues
        void ProcessResultQueue();

        bool CheckRequiredField(char const* table_name, char const* required_name);
        uint32 GetPingIntervall() { return m_pingIntervallms; }

        //function to ping database connections
        inline void Ping(const char *sql)
        {
            {
                SqlConnection::Lock guard(*m_pAsyncConn);
                delete m_pAsyncConn->Query(sql);
            }

            {
                SqlConnection::Lock guard(*m_pQueryConn);
                delete m_pQueryConn->Query(sql);
            }

        }

    protected:
        Database() : m_pQueryConn(NULL), m_pAsyncConn(NULL), m_pResultQueue(NULL), 
            m_threadBody(NULL), m_delayThread(NULL), m_logSQL(false), m_pingIntervallms(0)
        {}

        //factory method to create SqlConnection objects
        virtual SqlConnection * CreateConnection() = 0;
        //factory method to create SqlDelayThread objects
        virtual SqlDelayThread * CreateDelayThread();

        class TransHelper
        {
            public:
                TransHelper() : m_pTrans(NULL) {}
                ~TransHelper();

                SqlTransaction * init();
                SqlTransaction * get() const { return m_pTrans; }
                SqlTransaction * release();

            private:
                SqlTransaction * m_pTrans;
        };

        typedef ACE_TSS<Database::TransHelper> DBTransHelperTSS;
        Database::DBTransHelperTSS m_TransStorage;

        //DB connections
        SqlConnection * m_pQueryConn;
        SqlConnection * m_pAsyncConn;

        SqlResultQueue *    m_pResultQueue;                  ///< Transaction queues from diff. threads
        SqlDelayThread *    m_threadBody;                    ///< Pointer to delay sql executer (owned by m_delayThread)
        ACE_Based::Thread * m_delayThread;                   ///< Pointer to executer thread

    private:

        bool m_logSQL;
        std::string m_logsDir;
        uint32 m_pingIntervallms;
};
#endif
