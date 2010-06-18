using System;
using System.Collections.Generic;
using System.Text;

namespace Keyspace
{
    public class Status
    {
        public const int KEYSPACE_SUCCESS = 0;
        public const int KEYSPACE_API_ERROR = -1;

        public const int KEYSPACE_PARTIAL = -101;
        public const int KEYSPACE_FAILURE = -102;

        public const int KEYSPACE_NOMASTER = -201;
        public const int KEYSPACE_NOCONNECTION = -202;

        public const int KEYSPACE_MASTER_TIMEOUT = -301;
        public const int KEYSPACE_GLOBAL_TIMEOUT = -302;

        public const int KEYSPACE_NOSERVICE = -401;
        public const int KEYSPACE_FAILED = -402;

        public static string ToString(int status)
        {
            switch (status)
            {
                case KEYSPACE_SUCCESS:
                    return "KEYSPACE_SUCCESS";
                case KEYSPACE_API_ERROR:
                    return "KEYSPACE_API_ERROR";
                case KEYSPACE_PARTIAL:
                    return "KEYSPACE_PARTIAL";
                case KEYSPACE_FAILURE:
                    return "KEYSPACE_FAILURE";
                case KEYSPACE_NOMASTER:
                    return "KEYSPACE_NOMASTER";
                case KEYSPACE_NOCONNECTION:
                    return "KEYSPACE_NOCONNECTION";
                case KEYSPACE_MASTER_TIMEOUT:
                    return "KEYSPACE_MASTER_TIMEOUT";
                case KEYSPACE_GLOBAL_TIMEOUT:
                    return "KEYSPACE_GLOBAL_TIMEOUT";
                case KEYSPACE_NOSERVICE:
                    return "KEYSPACE_NOSERVICE";
                case KEYSPACE_FAILED:
                    return "KEYSPACE_FAILED";
            }

            return "<UNKNOWN>";
        }
    }
}
