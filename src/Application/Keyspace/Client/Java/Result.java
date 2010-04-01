package com.scalien.keyspace;

import java.util.TreeMap;

public class Result
{
	private SWIGTYPE_p_void cptr;
	
	public Result(SWIGTYPE_p_void cptr) {
		this.cptr = cptr;
	}
	
	public void finalize() {
		keyspace_client.Keyspace_ResultClose(cptr);
	}
	
	public String getKey() {
		return keyspace_client.Keyspace_ResultKey(cptr);
	}
	
	public String getValue() {
		return keyspace_client.Keyspace_ResultValue(cptr);
	}
	
	public void begin() {
		keyspace_client.Keyspace_ResultBegin(cptr);
	}
	
	public void next() {
		keyspace_client.Keyspace_ResultNext(cptr);
	}
	
	public boolean isEnd() {
		return keyspace_client.Keyspace_ResultIsEnd(cptr);
	}
	
	public int transportStatus() {
		return keyspace_client.Keyspace_ResultTransportStatus(cptr);
	}
	
	public int connectivityStatus() {
		return keyspace_client.Keyspace_ResultConnectivityStatus(cptr);
	}
	
	public int timeoutStatus() {
		return keyspace_client.Keyspace_ResultTimeoutStatus(cptr);
	}
	
	public int commandStatus() {
		return keyspace_client.Keyspace_ResultCommandStatus(cptr);
	}	
	
	public TreeMap<String, String> getKeyValues() {
		TreeMap<String, String> keyvals = new TreeMap<String, String>();
		for (begin(); !isEnd(); next())
			keyvals.put(getKey(), getValue());
			
		return keyvals;
	}
}
