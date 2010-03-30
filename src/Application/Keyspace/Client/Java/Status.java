package com.scalien.keyspace;

public class Status
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

