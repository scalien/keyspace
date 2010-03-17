import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;

class Status
{
	public final static int KEYSPACE_SUCCESS		= 0;
	public final static int KEYSPACE_API_ERROR		= -1;

	public final static int KEYSPACE_PARTIAL		= -101;
	public final static int KEYSPACE_FAILURE		= -102;

	public final static int KEYSPACE_NOMASTER		= -201;
	public final static int KEYSPACE_NOCONNECTION		= -202;

	public final static int KEYSPACE_MASTER_TIMEOUT		= -301;
	public final static int KEYSPACE_GLOBAL_TIMEOUT		= -302;

	public final static int KEYSPACE_NOSERVICE		= -401;
	public final static int KEYSPACE_FAILED			= -402;
	
	public static String toString(int status)
	{
		switch (status) {
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

class Result
{
	private SWIGTYPE_p_void cptr;
	
	public Result(SWIGTYPE_p_void cptr) {
		this.cptr = cptr;
	}
	
	public void finalize() {
		keyspace_client.ResultClose(cptr);
	}
	
	public String getKey() {
		return keyspace_client.ResultKey(cptr);
	}
	
	public String getValue() {
		return keyspace_client.ResultValue(cptr);
	}
	
	public void begin() {
		keyspace_client.ResultBegin(cptr);
	}
	
	public void next() {
		keyspace_client.ResultNext(cptr);
	}
	
	public boolean isEnd() {
		return keyspace_client.ResultIsEnd(cptr);
	}
	
	public int transportStatus() {
		return keyspace_client.ResultTransportStatus(cptr);
	}
	
	public int connectivityStatus() {
		return keyspace_client.ResultConnectivityStatus(cptr);
	}
	
	public int timeoutStatus() {
		return keyspace_client.ResultTimeoutStatus(cptr);
	}
	
	public int commandStatus() {
		return keyspace_client.ResultCommandStatus(cptr);
	}
	
}

class ListParams
{
	ListParams() {
		prefix = "";
		startKey = "";
		limit = 0;
		skip = false;
		forward = true;
	}
	
	public ListParams setPrefix(String prefix) {
		this.prefix = prefix;
		return this;
	}
	
	public ListParams setStartKey(String startKey) {
		this.startKey = startKey;
		return this;
	}
	
	public ListParams setLimit(long limit) {
		this.limit = limit;
		return this;
	}
	
	public ListParams setSkip(boolean skip) {
		this.skip = skip;
		return this;
	}
	
	public ListParams setForward(boolean forward) {
		this.forward = forward;
		return this;
	}
	
	public String prefix;
	public String startKey;
	public long limit;
	public boolean skip;
	public boolean forward;
}

public class Client
{
	static {
		System.loadLibrary("keyspace_client");
	}

	private SWIGTYPE_p_void cptr;
	private Result result;
	
	public Client(String[] nodes) {
		cptr = keyspace_client.Create();
		result = null;
		
		NodeParams nodeParams = new NodeParams(nodes.length);
		for (int i = 0; i < nodes.length; i++) {
			nodeParams.AddNode(nodes[i]);
		}
		
		int status = keyspace_client.Init(cptr, nodeParams);
		System.out.println("status = " + status);
		nodeParams.Close();
	}

	public void finalize() {
		keyspace_client.Destroy(cptr);
	}
	
	public void setGlobalTimeout(long timeout) {
		keyspace_client.SetGlobalTimeout(cptr, BigInteger.valueOf(timeout));
	}
	
	public void setMasterTimeout(long timeout) {
		keyspace_client.SetMasterTimeout(cptr, BigInteger.valueOf(timeout));
	}
	
	public long getGlobalTimeout() {
		BigInteger bi = keyspace_client.GetGlobalTimeout(cptr);
		return bi.longValue();
	}
	
	public long getMasterTimeout() {
		BigInteger bi = keyspace_client.GetMasterTimeout(cptr);
		return bi.longValue();
	}
	
	public int getMaster() {
		return keyspace_client.GetMaster(cptr);
	}
	
	public void distributeDirty(boolean dd) {
		keyspace_client.DistributeDirty(cptr, dd);
	}
	
	public Result getResult() {
		return result;
	}
	
	public String get(String key) {
		int status = keyspace_client.Get(cptr, key);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return null;
		}
		
		if (isBatched())
			return null;
				
		result = new Result(keyspace_client.GetResult(cptr));
		return result.getValue();
	}
	
	public String dirtyGet(String key) {
		int status = keyspace_client.DirtyGet(cptr, key);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return null;
		}
		
		if (isBatched())
			return null;
				
		result = new Result(keyspace_client.GetResult(cptr));
		return result.getValue();
	}
	
	public long count(ListParams params) {
		return count(params.prefix, params.startKey, params.limit, params.skip, params.forward);
	}
	
	public long count(String prefix, String startKey, long limit, boolean skip, boolean forward) {
		int status = keyspace_client.Count(cptr, prefix, startKey, BigInteger.valueOf(limit), skip, forward);
		result = new Result(keyspace_client.GetResult(cptr));
		if (status < 0)
			return -1;
		
		try {
			return Long.parseLong(result.getValue());
		} catch (NumberFormatException nfe) {
			return -1;
		}
	}

	public long dirtyCount(ListParams params) {
		return dirtyCount(params.prefix, params.startKey, params.limit, params.skip, params.forward);
	}
	
	public long dirtyCount(String prefix, String startKey, long limit, boolean skip, boolean forward) {
		int status = keyspace_client.DirtyCount(cptr, prefix, startKey, BigInteger.valueOf(limit), skip, forward);
		result = new Result(keyspace_client.GetResult(cptr));
		if (status < 0)
			return -1;
		
		try {
			return Long.parseLong(result.getValue());
		} catch (NumberFormatException nfe) {
			return -1;
		}
	}

	public ArrayList<String> listKeys(ListParams params) {
		return listKeys(params.prefix, params.startKey, params.limit, params.skip, params.forward);
	}
	
	public ArrayList<String> listKeys(String prefix, String startKey, long limit, boolean skip, boolean forward) {
		int status = keyspace_client.ListKeys(cptr, prefix, startKey, BigInteger.valueOf(limit), skip, forward);
		result = new Result(keyspace_client.GetResult(cptr));
		if (status < 0)
			return null;
		
		ArrayList<String> keys = new ArrayList<String>();
		for (result.begin(); !result.isEnd(); result.next())
			keys.add(result.getKey());
		
		return keys;
	}

	public HashMap<String, String> listKeyValues(ListParams params) {
		return listKeyValues(params.prefix, params.startKey, params.limit, params.skip, params.forward);
	}

	public HashMap<String, String> listKeyValues(String prefix, String startKey, long limit, boolean skip, boolean forward) {
		int status = keyspace_client.ListKeyValues(cptr, prefix, startKey, BigInteger.valueOf(limit), skip, forward);
		result = new Result(keyspace_client.GetResult(cptr));
		if (status < 0)
			return null;
		
		HashMap<String, String> keyvals = new HashMap<String, String>();
		for (result.begin(); !result.isEnd(); result.next())
			keyvals.put(result.getKey(), result.getValue());
		
		return keyvals;
	}
	
	
	public int set(String key, String value) {
		int status = keyspace_client.Set(cptr, key, value);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return status;
		}
		
		if (isBatched())
			return status;
				
		result = new Result(keyspace_client.GetResult(cptr));
		return status;
	}
	
	public String testAndSet(String key, String test, String value) {
		int status = keyspace_client.TestAndSet(cptr, key, test, value);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return null;
		}
		
		if (isBatched())
			return null;
				
		result = new Result(keyspace_client.GetResult(cptr));
		return result.getValue();		
	}
	
	public long add(String key, long num) {
		int status = keyspace_client.Add(cptr, key, num);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return -1;
		}
		
		if (isBatched())
			return -1;
		
		result = new Result(keyspace_client.GetResult(cptr));

		try {
			return Long.parseLong(result.getValue());
		} catch (NumberFormatException nfe) {
			return -1;
		}
	}
	
	public int delete(String key) {
		int status = keyspace_client.Delete(cptr, key);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return status;
		}
		
		if (isBatched())
			return status;
		
		result = new Result(keyspace_client.GetResult(cptr));
		return status;
	}
	
	public String remove(String key) {
		int status = keyspace_client.Remove(cptr, key);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return null;
		}
		
		if (isBatched())
			return null;
		
		result = new Result(keyspace_client.GetResult(cptr));
		return result.getValue();
	}
	
	public int rename(String src, String dst) {
		int status = keyspace_client.Rename(cptr, src, dst);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return status;
		}
		
		if (isBatched())
			return status;
		
		result = new Result(keyspace_client.GetResult(cptr));
		return status;
	}
	
	public int prune(String prefix) {
		int status = keyspace_client.Prune(cptr, prefix);
		if (status < 0) {
			result = new Result(keyspace_client.GetResult(cptr));
			return status;
		}
		
		if (isBatched())
			return status;
		
		result = new Result(keyspace_client.GetResult(cptr));
		return status;
	}

	public int begin() {
		return keyspace_client.Begin(cptr);
	}
	
	public int submit() {
		int status = keyspace_client.Submit(cptr);
		result = new Result(keyspace_client.GetResult(cptr));
		return status;
	}
	
	public int cancel() {
		return keyspace_client.Cancel(cptr);
	}

	private boolean isBatched() {
		return keyspace_client.IsBatched(cptr);
	}
	
	private static void setTrace(boolean trace) {
		keyspace_client.SetTrace(trace);
	}
	
	public static void main(String[] args) {
		String[] nodes = {"127.0.0.1:7080"};
		setTrace(true);
		Client ks = new Client(nodes);
		String hol = ks.get("hol");
		System.out.println(hol);
		
		ArrayList<String> keys = ks.listKeys("", "", 0, false, true);
		for (String key : keys)
			System.out.println(key);
		
		HashMap<String, String> keyvals = ks.listKeyValues("", "", 0, false, true);
		Collection c = keyvals.keySet();
		Iterator it = c.iterator();
		while (it.hasNext()) {
			Object key = it.next();
			String value = keyvals.get(key);
			System.out.println(key + " => " + value);
		}
		
		long cnt = ks.count(new ListParams().setLimit(100));
		System.out.println(cnt);
	}
}