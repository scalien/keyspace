using System;
using System.Collections.Generic;
using System.Text;

using Keyspace;

namespace ConsoleApplication1
{
    class Program
    {
        static void Main(string[] args)
        {
            string[] nodes = { "192.168.1.105:7080" };
            Client client = new Client(nodes);
            client.Set("hol", "peru");
            string hol = client.Get("hol");
            Console.WriteLine(hol);
        }
    }
}
