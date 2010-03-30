package com.scalien.keyspace;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;


public class Client
{
	static {
		System.loadLibrary("keyspace_client");
	}

	private SWIGTYPE_p_void cptr;
	private Result result;
	
	public Client(String[] nodes) {
		cptr = keyspace_client.Keyspace_Create();
		result = null;
		
		Keyspace_NodeParams nodeParams = new Keyspace_NodeParams(nodes.length);
		for (int i = 0; i < nodes.length; i++) {
			nodeParams.AddNode(nodes[i]);
		}
		
		int status = keyspace_client.Keyspace_Init(cptr, nodeParams);
		nodeParams.Close();
	}

	public void finalize() {
		keyspace_client.Keyspace_Destroy(cptr);
	}
	
	public void setGlobalTimeout(long timeout) {
		keyspace_client.Keyspace_SetGlobalTimeout(cptr, BigInteger.valueOf(timeout));
	}
	
	public void setMasterTimeout(long timeout) {
		keyspace_client.Keyspace_SetMasterTimeout(cptr, BigInteger.valueOf(timeout));
	}
	
	public long getGlobalTimeout() {
		BigInteger bi = keyspace_client.Keyspace_GetGlobalTimeout(cptr);
		return bi.longValue();
	}
	
	public long getMasterTimeout() {
		BigInteger bi = keyspace_client.Keyspace_GetMasterTimeout(cptr);
		return bi.longValue();
	}
	
	public int getMaster() {
		return keyspace_client.Keyspace_GetMaster(cptr);
	}
	
	public void distributeDirty(boolean dd) {
		keyspace_client.Keyspace_DistributeDirty(cptr, dd);
	}
	
	public Result getResult() {
		return result;
	}
	
	public String get(String key) throws KeyspaceException {
		int status = keyspace_client.Keyspace_Get(cptr, key);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return null;
				
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return result.getValue();
	}
	
	public String dirtyGet(String key) throws KeyspaceException {
		int status = keyspace_client.Keyspace_DirtyGet(cptr, key);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return null;
				
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return result.getValue();
	}
	
	public long count(ListParams params) throws KeyspaceException {
		return count(params.prefix, params.startKey, params.limit, params.skip, params.forward);
	}
	
	public long count(String prefix, String startKey, long limit, boolean skip, boolean forward) throws KeyspaceException {
		int status = keyspace_client.Keyspace_Count(cptr, prefix, startKey, BigInteger.valueOf(limit), skip, forward);
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		if (status < 0)
			throw new KeyspaceException(Status.toString(status));
		
		try {
			return Long.parseLong(result.getValue());
		} catch (NumberFormatException nfe) {
			throw new KeyspaceException(Status.toString(Status.KEYSPACE_API_ERROR));
		}
	}

	public long dirtyCount(ListParams params) throws KeyspaceException {
		return dirtyCount(params.prefix, params.startKey, params.limit, params.skip, params.forward);
	}
	
	public long dirtyCount(String prefix, String startKey, long limit, boolean skip, boolean forward) throws KeyspaceException {
		int status = keyspace_client.Keyspace_DirtyCount(cptr, prefix, startKey, BigInteger.valueOf(limit), skip, forward);
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		if (status < 0)
			throw new KeyspaceException(Status.toString(status));
		
		try {
			return Long.parseLong(result.getValue());
		} catch (NumberFormatException nfe) {
			throw new KeyspaceException(Status.toString(Status.KEYSPACE_API_ERROR));
		}
	}

	public ArrayList<String> listKeys(ListParams params) throws KeyspaceException {
		return listKeys(params.prefix, params.startKey, params.limit, params.skip, params.forward);
	}
	
	public ArrayList<String> listKeys(String prefix, String startKey, long limit, boolean skip, boolean forward) throws KeyspaceException {
		int status = keyspace_client.Keyspace_ListKeys(cptr, prefix, startKey, BigInteger.valueOf(limit), skip, forward);
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		if (status < 0)
			throw new KeyspaceException(Status.toString(status));
		
		ArrayList<String> keys = new ArrayList<String>();
		for (result.begin(); !result.isEnd(); result.next())
			keys.add(result.getKey());
		
		return keys;
	}

	public TreeMap<String, String> listKeyValues(ListParams params) throws KeyspaceException {
		return listKeyValues(params.prefix, params.startKey, params.limit, params.skip, params.forward);
	}

	public TreeMap<String, String> listKeyValues(String prefix, String startKey, long limit, boolean skip, boolean forward) throws KeyspaceException {
		int status = keyspace_client.Keyspace_ListKeyValues(cptr, prefix, startKey, BigInteger.valueOf(limit), skip, forward);
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		if (status < 0)
			throw new KeyspaceException(Status.toString(status));
		
		TreeMap<String, String> keyvals = new TreeMap<String, String>();
		for (result.begin(); !result.isEnd(); result.next())
			keyvals.put(result.getKey(), result.getValue());
		
		return keyvals;
	}
	
	
	public int set(String key, String value) throws KeyspaceException {
		int status = keyspace_client.Keyspace_Set(cptr, key, value);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return status;
				
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return status;
	}
	
	public String testAndSet(String key, String test, String value) throws KeyspaceException {
		int status = keyspace_client.Keyspace_TestAndSet(cptr, key, test, value);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return null;
				
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return result.getValue();		
	}
	
	public long add(String key, long num) throws KeyspaceException {
		int status = keyspace_client.Keyspace_Add(cptr, key, num);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return 0;
		
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));

		try {
			return Long.parseLong(result.getValue());
		} catch (NumberFormatException nfe) {
			throw new KeyspaceException(Status.toString(Status.KEYSPACE_API_ERROR));
		}
	}
	
	public int delete(String key) throws KeyspaceException {
		int status = keyspace_client.Keyspace_Delete(cptr, key);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return status;
		
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return status;
	}
	
	public String remove(String key) throws KeyspaceException {
		int status = keyspace_client.Keyspace_Remove(cptr, key);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return null;
		
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return result.getValue();
	}
	
	public int rename(String src, String dst) throws KeyspaceException {
		int status = keyspace_client.Keyspace_Rename(cptr, src, dst);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return status;
		
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return status;
	}
	
	public int prune(String prefix) throws KeyspaceException {
		int status = keyspace_client.Keyspace_Prune(cptr, prefix);
		if (status < 0) {
			result = new Result(keyspace_client.Keyspace_GetResult(cptr));
			throw new KeyspaceException(Status.toString(status));
		}
		
		if (isBatched())
			return status;
		
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return status;
	}

	public int begin() {
		return keyspace_client.Keyspace_Begin(cptr);
	}
	
	public int submit() {
		int status = keyspace_client.Keyspace_Submit(cptr);
		result = new Result(keyspace_client.Keyspace_GetResult(cptr));
		return status;
	}
	
	public int cancel() {
		return keyspace_client.Keyspace_Cancel(cptr);
	}

	private boolean isBatched() {
		return keyspace_client.Keyspace_IsBatched(cptr);
	}
	
	private static void setTrace(boolean trace) {
		keyspace_client.Keyspace_SetTrace(trace);
	}
	
	public static void main(String[] args) {
		try {
			String[] nodes = {"127.0.0.1:7080"};
			//setTrace(true);
			Client ks = new Client(nodes);
			String hol = ks.get("hol");
			System.out.println(hol);
			
			ArrayList<String> keys = ks.listKeys("", "", 0, false, true);
			for (String key : keys)
				System.out.println(key);
			
			Map<String, String> keyvals = ks.listKeyValues("", "", 0, false, true);
			Collection c = keyvals.keySet();
			Iterator it = c.iterator();
			while (it.hasNext()) {
				Object key = it.next();
				String value = keyvals.get(key);
				System.out.println(key + " => " + value);
			}
			
			long cnt = ks.count(new ListParams().setLimit(100));
			System.out.println(cnt);
		} catch (KeyspaceException e) {
			System.out.println(e.getMessage());
		}
	}
}