using System;
using System.Collections.Generic;
using System.Text;

namespace Keyspace
{
    public class Result
    {
        private SWIGTYPE_p_void cptr;

        public Result(SWIGTYPE_p_void cptr)
        {
            this.cptr = cptr;
        }

        ~Result()
        {
            keyspace_client.Keyspace_ResultClose(cptr);
        }

        public string GetKey()
        {
            return keyspace_client.Keyspace_ResultKey(cptr);
        }

        public string GetValue()
        {
            return keyspace_client.Keyspace_ResultValue(cptr);
        }

        public void Begin()
        {
            keyspace_client.Keyspace_ResultBegin(cptr);
        }

        public void Next()
        {
            keyspace_client.Keyspace_ResultNext(cptr);
        }

        public bool IsEnd()
        {
            return keyspace_client.Keyspace_ResultIsEnd(cptr);
        }

        public int GetTransportStatus()
        {
            return keyspace_client.Keyspace_ResultTransportStatus(cptr);
        }

        public int GetConnectivityStatus()
        {
            return keyspace_client.Keyspace_ResultConnectivityStatus(cptr);
        }

        public int GetTimeoutStatus()
        {
            return keyspace_client.Keyspace_ResultTimeoutStatus(cptr);
        }

        public int GetCommandStatus()
        {
            return keyspace_client.Keyspace_ResultCommandStatus(cptr);
        }

        public Dictionary<string, string> GetKeyValues()
        {
            Dictionary<string, string> keyvals = new Dictionary<string, string>();
            for (Begin(); !IsEnd(); Next())
                keyvals.Add(GetKey(), GetValue());

            return keyvals;
        }
    }
}
