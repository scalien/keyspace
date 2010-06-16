using System;
using System.Collections.Generic;
using System.Text;

namespace Keyspace
{
    public class Exception : ApplicationException
    {
        public Exception(string msg)
        {
            //base(msg);   
        }
    }

    public class Client
    {
        private SWIGTYPE_p_void cptr;
        private Result result;

        public Client(string[] nodes)
        {
            cptr = keyspace_client.Keyspace_Create();
            result = null;

            Keyspace_NodeParams nodeParams = new Keyspace_NodeParams(nodes.Length);
            for (int i = 0; i < nodes.Length; i++)
                nodeParams.AddNode(nodes[i]);

            int status = keyspace_client.Keyspace_Init(cptr, nodeParams);
            nodeParams.Close();
        }

        ~Client()
        {
            keyspace_client.Keyspace_Destroy(cptr);
        }

        public void SetGlobalTimeout(ulong timeout)
        {
            keyspace_client.Keyspace_SetGlobalTimeout(cptr, timeout);
        }

        public void SetMasterTimeout(ulong timeout)
        {
            keyspace_client.Keyspace_SetMasterTimeout(cptr, timeout);
        }

        public ulong GetGlobalTimeout()
        {
            return keyspace_client.Keyspace_GetGlobalTimeout(cptr);
        }

        public ulong GetMasterTimeout()
        {
            return keyspace_client.Keyspace_GetMasterTimeout(cptr);
        }

        public int GetMaster()
        {
            return keyspace_client.Keyspace_GetMaster(cptr);
        }

        public void DistributeDirty(bool dd)
        {
            keyspace_client.Keyspace_DistributeDirty(cptr, dd);
        }

        public Result GetResult()
        {
            return result;
        }

        public string Get(string key)
        {
            int status = keyspace_client.Keyspace_Get(cptr, key);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return null;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            return result.GetValue();
        }

        public string DirtyGet(string key)
        {
            int status = keyspace_client.Keyspace_DirtyGet(cptr, key);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return null;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            return result.GetValue();
        }

        public long Count(ListParam lp)
        {
            return Count(lp.prefix, lp.startKey, lp.count, lp.skip, lp.forward);
        }

        public long Count(string prefix, string startKey, ulong count, bool skip, bool forward)
        {
            int status = keyspace_client.Keyspace_Count(cptr, prefix, startKey, count, skip, forward);
            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            if (status < 0)
                throw new Exception(Status.ToString(status));

            try
            {
                return long.Parse(result.GetValue());
            }
            catch (FormatException)
            {
                throw new Exception(Status.ToString(Status.KEYSPACE_API_ERROR));
            }
        }

        public long DirtyCount(ListParam lp)
        {
            return DirtyCount(lp.prefix, lp.startKey, lp.count, lp.skip, lp.forward);
        }

        public long DirtyCount(string prefix, string startKey, ulong count, bool skip, bool forward)
        {
            int status = keyspace_client.Keyspace_DirtyCount(cptr, prefix, startKey, count, skip, forward);
            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            if (status < 0)
                throw new Exception(Status.ToString(status));

            try
            {
                return long.Parse(result.GetValue());
            }
            catch (FormatException)
            {
                throw new Exception(Status.ToString(Status.KEYSPACE_API_ERROR));
            }
        }

        public List<string> ListKeys(ListParam lp)
        {
            return ListKeys(lp.prefix, lp.startKey, lp.count, lp.skip, lp.forward);
        }

        public List<string> ListKeys(string prefix, string startKey, ulong count, bool skip, bool forward)
        {
            int status = keyspace_client.Keyspace_ListKeys(cptr, prefix, startKey, count, skip, forward);
            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            if (status < 0)
                throw new Exception(Status.ToString(status));

            List<string> keys = new List<string>();
            for (result.Begin(); !result.IsEnd(); result.Next())
                keys.Add(result.GetKey());

            return keys;
        }

        public List<string> DirtyListKeys(ListParam lp)
        {
            return DirtyListKeys(lp.prefix, lp.startKey, lp.count, lp.skip, lp.forward);
        }

        public List<string> DirtyListKeys(string prefix, string startKey, ulong count, bool skip, bool forward)
        {
            int status = keyspace_client.Keyspace_DirtyListKeys(cptr, prefix, startKey, count, skip, forward);
            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            if (status < 0)
                throw new Exception(Status.ToString(status));

            List<string> keys = new List<string>();
            for (result.Begin(); !result.IsEnd(); result.Next())
                keys.Add(result.GetKey());

            return keys;
        }

        public Dictionary<string, string> DirtyListKeyValues(ListParam lp)
        {
            return DirtyListKeyValues(lp.prefix, lp.startKey, lp.count, lp.skip, lp.forward);
        }

        public Dictionary<string, string> DirtyListKeyValues(string prefix, string startKey, ulong count, bool skip, bool forward)
        {
            int status = keyspace_client.Keyspace_DirtyListKeyValues(cptr, prefix, startKey, count, skip, forward);
            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            if (status < 0)
                throw new Exception(Status.ToString(status));

            Dictionary<string, string> keyValues = new Dictionary<string, string>();
            for (result.Begin(); !result.IsEnd(); result.Next())
                keyValues.Add(result.GetKey(), result.GetValue());

            return keyValues;
        }

        public Dictionary<string, string> ListKeyValues(ListParam lp)
        {
            return ListKeyValues(lp.prefix, lp.startKey, lp.count, lp.skip, lp.forward);
        }

        public Dictionary<string, string> ListKeyValues(string prefix, string startKey, ulong count, bool skip, bool forward)
        {
            int status = keyspace_client.Keyspace_ListKeyValues(cptr, prefix, startKey, count, skip, forward);
            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            if (status < 0)
                throw new Exception(Status.ToString(status));

            Dictionary<string, string> keyValues = new Dictionary<string, string>();
            for (result.Begin(); !result.IsEnd(); result.Next())
                keyValues.Add(result.GetKey(), result.GetValue());

            return keyValues;
        }

        public int Set(string key, string value)
        {
            int status = keyspace_client.Keyspace_Set(cptr, key, value);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return status;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            return status;
        }

        public string TestAndSet(string key, string test, string value)
        {
            int status = keyspace_client.Keyspace_TestAndSet(cptr, key, test, value);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return null;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            return result.GetValue();
        }

        public long Add(string key, long num)
        {
            int status = keyspace_client.Keyspace_Add(cptr, key, num);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return 0;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            try
            {
                return long.Parse(result.GetValue());
            }
            catch (FormatException)
            {
                throw new Exception(Status.ToString(Status.KEYSPACE_API_ERROR));
            }
        }

        public int Delete(string key)
        {
            int status = keyspace_client.Keyspace_Delete(cptr, key);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return status;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            return status;
        }

        public string Remove(string key)
        {
            int status = keyspace_client.Keyspace_Remove(cptr, key);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return null;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            return result.GetValue();
        }

        public int Rename(string src, string dst)
        {
            int status = keyspace_client.Keyspace_Rename(cptr, src, dst);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return status;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            return status;
        }

        public int Prune(string prefix)
        {
            int status = keyspace_client.Keyspace_Prune(cptr, prefix);
            if (status < 0)
            {
                result = new Result(keyspace_client.Keyspace_GetResult(cptr));
                throw new Exception(Status.ToString(status));
            }

            if (IsBatched())
                return status;

            result = new Result(keyspace_client.Keyspace_GetResult(cptr));
            return status;
        }

        public int Begin()
        {
            return keyspace_client.Keyspace_Begin(cptr);
        }

        public int Submit()
        {
            return keyspace_client.Keyspace_Submit(cptr);
        }

        public int Cancel()
        {
            return keyspace_client.Keyspace_Cancel(cptr);
        }

        public bool IsBatched()
        {
            return keyspace_client.Keyspace_IsBatched(cptr);
        }

        public static void SetTrace(bool trace)
        {
            keyspace_client.Keyspace_SetTrace(trace);
        }
    }
}
