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


        public bool IsBatched()
        {
            return keyspace_client.Keyspace_IsBatched(cptr);
        }
    }
}
