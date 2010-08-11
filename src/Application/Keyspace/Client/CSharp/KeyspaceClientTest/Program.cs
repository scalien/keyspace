using System;
using System.Collections.Generic;
using System.Text;

using Keyspace;

namespace KeyspaceClientTest
{
    class Program
    {
        static void Main(string[] args)
        {
            string[] nodes = { "localhost:7080" };
            Client client = new Client(nodes);
            //client.Set("hol", "peru");
            //string hol = client.Get("hol");
            //Console.WriteLine(hol);

            //List<string> keys = client.ListKeys("", "", 0, false, true);
            //foreach (string key in keys)
            //    Console.WriteLine(key);

            //Dictionary<string, string> keyValues = client.ListKeyValues("", "", 0, false, true);
            //foreach (KeyValuePair<string, string> keyValue in keyValues)
            //    Console.WriteLine(keyValue.Key + ", " + keyValue.Value);

            client.Prune("");
            client.ClearExpiries();
            
//            client.Set("a1", "b1");
            client.SetExpiry("a1", 1);
            client.ClearExpiries();
        }
    }
}
