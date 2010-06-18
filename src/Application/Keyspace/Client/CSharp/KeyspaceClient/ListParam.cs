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

        public ListParam SetPrefix(string prefix)
        {
            this.prefix = prefix;
            return this;
        }

        public ListParam SetStartKey(string startKey)
        {
            this.startKey = startKey;
            return this;
        }

        public ListParam SetCount(ulong count)
        {
            this.count = count;
            return this;
        }

        public ListParam SetSkip(bool skip)
        {
            this.skip = skip;
            return this;
        }

        public ListParam SetForward(bool forward)
        {
            this.forward = forward;
            return this;
        }
    }
}
