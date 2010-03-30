package com.scalien.keyspace;

public class ListParams
{
	public ListParams() {
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
