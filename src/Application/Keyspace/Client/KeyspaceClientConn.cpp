#include "KeyspaceClientConn.h"
#include "KeyspaceClient.h"
#include "KeyspaceClientConsts.h"
#include "KeyspaceCommand.h"
#include "KeyspaceResponse.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientReq.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientResp.h"
#include "Framework/PaxosLease/PLeaseConsts.h"

#define RECONNECT_TIMEOUT	2000
#define GETMASTER_TIMEOUT	1000

using namespace Keyspace;

ClientConn::ClientConn(Client &client, int nodeID_,
const Endpoint &endpoint_, uint64_t timeout_) :
client(client),
endpoint(endpoint_),
onGetMasterTimeout(this, &ClientConn::OnGetMaster),
getMasterTimeout(&onGetMasterTimeout)
{
	nodeID = nodeID_;
	getMasterTime = 0;
	timeout = timeout_;
	getMasterTimeout.SetDelay(GETMASTER_TIMEOUT);
	Connect();
}

void ClientConn::Connect()
{
	TCPConn<KEYSPACE_BUF_SIZE>::Connect(endpoint, RECONNECT_TIMEOUT);
}

void ClientConn::Send(Command &cmd)
{
	Log_Trace("nodeID = %d, cmd = %.*s",
			  nodeID, MIN(cmd.msg.length, 40), cmd.msg.buffer);

	cmd.nodeID = nodeID;	
	Write(cmd.msg.buffer, cmd.msg.length);
	submit = false;
}

void ClientConn::SendSubmit()
{
	if (!submit)
	{
		submit = true;
		Write("1:*", 3);
	}
}

void ClientConn::SendGetMaster()
{
	Command* cmd;
	
	if (state == CONNECTED)
	{
		cmd = client.CreateCommand(KEYSPACECLIENT_GET_MASTER, 0, NULL);
		getMasterCommands.Append(cmd);
		Send(*cmd);
		EventLoop::Reset(&getMasterTimeout);
	}
	else
		ASSERT_FAIL();
}

void ClientConn::OnGetMaster()
{
	Log_Trace();

	if (EventLoop::Now() - getMasterTime > MAX_LEASE_TIME)
	{
		Log_Trace();
		
		OnClose();
		Connect();
		return;
	}
	
	SendGetMaster();
}

bool ClientConn::ProcessCommand(Response* resp)
{
	Command**	it;
	
	if (resp->type == KEYSPACECLIENT_NOT_MASTER)
	{
		Log_Trace("NOTMASTER");
		client.SetMaster(-1, nodeID);
		return true;
	}
	
	for (it = client.result->commands.Head(); it != NULL; /* advanced in body */)
	{
		Command* cmd = *it;
		
		if (cmd->cmdID == resp->id)
		{
			Log_Trace("status = %d, id = %lu",
					  (int) cmd->status, (unsigned long) cmd->cmdID);
			
			// with LIST style commands there can be more
			// than one response with the same id
			if (cmd->IsList())
			{
				// key.length == 0 means end of the list response
				if (resp->key.length == 0)
				{
					client.result->numCompleted++;
					cmd->status = KEYSPACE_SUCCESS;
				}
				else
					client.result->AppendCommandResponse(cmd, resp);
			}
			else
			{
				if (resp->type == KEYSPACECLIENT_OK)
					cmd->status = KEYSPACE_SUCCESS;
				else
					cmd->status = KEYSPACE_FAILED;

				client.result->AppendCommandResponse(cmd, resp);
				client.result->numCompleted++;
			}
			
			return true;
		}
		else
			it = client.result->commands.Next(it);
	}
	
	return false;
}

bool ClientConn::ProcessGetMaster(Response* resp)
{
	Command**	it;
	unsigned	nread;
	int			master;
	
	for (it = getMasterCommands.Head(); it != NULL; /* advanced in body */)
	{
		Command* cmd = *it;
		
		if (cmd->cmdID != resp->id)
		{
			it = getMasterCommands.Next(it);
			continue;
		}

		//cmd->status = resp->status;
		
		Log_Trace("status = %d, id = %lu",
				  (int) cmd->status, (unsigned long) cmd->cmdID);

		// GET_MASTER is a special case, because it is only used internally
		if (cmd->type != KEYSPACECLIENT_GET_MASTER)
			ASSERT_FAIL();

		getMasterTime = EventLoop::Now();
		Log_Trace("getMasterTime = %" PRIu64, getMasterTime);
		
		if (resp->type == KEYSPACECLIENT_OK)
		{
			master = (int) strntoint64(resp->value.buffer, 
											   resp->value.length,
											   &nread);

			client.SetMaster(master, nodeID);
			// TODO: protocol violation
			//if (nread != resp->value.length)
			//	resp->status = KEYSPACE_ERROR;
		}
		else
			client.SetMaster(-1, nodeID);
			
		getMasterCommands.Remove(it);
		delete cmd;
		
		return true;
	}
	
	return false;
}

bool ClientConn::ProcessResponse(Response* resp)
{
	switch (resp->id % 10)
	{
	case KEYSPACE_MOD_GETMASTER:
		return ProcessGetMaster(resp);

	case KEYSPACE_MOD_SAFE_COMMAND:
	case KEYSPACE_MOD_DIRTY_COMMAND:
		return ProcessCommand(resp);
	}

	ASSERT_FAIL();
	
	return true;
}

Endpoint& ClientConn::GetEndpoint()
{
	return endpoint;
}

void ClientConn::OnMessageRead(const ByteString& msg)
{
	Response	*resp;
	
	Log_Trace();
	
	resp = new Response;
	if (resp->Read(msg))
	{
		if (!ProcessResponse(resp))
			delete resp;
	}
	else
		delete resp;
}

void ClientConn::OnWrite()
{
	TCPConn<KEYSPACE_BUF_SIZE>::OnWrite();

	if (client.master == nodeID)
	{
		if (client.safeCommands.Length() > 0)
			client.SendCommand(this, client.safeCommands);
		else
			SendSubmit();
	}
	else if (client.dirtyCommands.Length() > 0)
		client.SendCommand(this, client.dirtyCommands);
}

void ClientConn::OnClose()
{
	Log_Trace();
	
	Command**	it;
	Command*	cmd;
	bool		connected;

	// delete getmaster requests
	for (it = getMasterCommands.Head(); it != NULL; /* advanced in body */)
	{
		cmd = *it;
		delete cmd;
		it = getMasterCommands.Remove(it);
	}

	if (state == CONNECTED)
	{
		for (it = client.result->commands.Head(); it != NULL;  it = client.result->commands.Next(it))
		{
			cmd = *it;
			if (cmd->status == KEYSPACE_NOSERVICE && cmd->nodeID == nodeID)
			{
				cmd->nodeID = -1;
				cmd->ClearResponse();
				if (cmd->IsDirty())
					client.dirtyCommands.Append(cmd);
				else
					client.safeCommands.Append(cmd);
			}
		}
	}
	
	// close the socket here
	Close();

	EventLoop::Remove(&getMasterTimeout);
	EventLoop::Reset(&connectTimeout);

	client.SetMaster(-1, nodeID);
		
	// TODO: move this to Client
	connected = false;
	for (int i = 0; i < client.numConns; i++)
	{
		if (client.conns[i]->GetState() != ClientConn::DISCONNECTED)
			connected = true;
	}
	if (!connected)
		client.connectivityStatus = KEYSPACE_NOCONNECTION;
}

void ClientConn::OnConnect()
{
	Log_Trace();
	
	TCPConn<KEYSPACE_BUF_SIZE>::OnConnect();
	AsyncRead();
	SendGetMaster();
	if (client.connectivityStatus == KEYSPACE_NOCONNECTION)
		client.connectivityStatus = KEYSPACE_NOMASTER;
}


void ClientConn::OnConnectTimeout()
{
	Log_Trace();
	
	OnClose();
	Connect();
}
