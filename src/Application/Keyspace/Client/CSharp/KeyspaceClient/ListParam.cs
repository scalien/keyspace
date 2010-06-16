using System;
using System.Collections.Generic;
using System.Text;

namespace Keyspace
{
    public class ListParam
    {
        public string prefix;
        public string startKey;
        public ulong count;
        public bool skip;
        public bool forward;

        public ListParam()
        {
            prefix = "";
            startKey = "";
            count = 0;
            skip = false;
            forward = true;
        }

        public ListParam setPrefix(string prefix)
        {
            this.prefix = prefix;
            return this;
        }

        public ListParam setStartKey(string startKey)
        {
            this.startKey = startKey;
            return this;
        }

        public ListParam setCount(ulong count)
        {
            this.count = count;
            return this;
        }

        public ListParam setSkip(bool skip)
        {
            this.skip = skip;
            return this;
        }

        public ListParam setForward(bool forward)
        {
            this.forward = forward;
            return this;
        }
    }
}
