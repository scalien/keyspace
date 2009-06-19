#include "KeyspaceConn.h"
#include "KeyspaceServer.h"

KeyspaceConn::KeyspaceConn()
:	onConnectionTimeout(this, &KeyspaceConn::OnConnectionTimeout),
	connectionTimeout(KEYSPACE_CONN_TIMEOUT, &onConnectionTimeout)
{
	server = NULL;
}

void KeyspaceConn::Init(KeyspaceDB* kdb_, KeyspaceServer* server_)
{
	Log_Trace();
	
	TCPConn<>::Init();
	KeyspaceService::Init(kdb_);
	
	server = server_;
	closeAfterSend = false;
	
	AsyncRead();
	
//	EventLoop::Reset(&connectionTimeout);
}

void KeyspaceConn::OnComplete(KeyspaceOp* op, bool final)
{
//	Log_Trace();
	
	data.Clear();
	
	if (state != DISCONNECTED)
	{
		if (!op->status && final && op->MasterOnly() && !kdb->IsMaster())
		{
			resp.NotMaster(op->cmdID);
			resp.Write(data);
		}
		else
		{
			if (op->type == KeyspaceOp::GET ||
				op->type == KeyspaceOp::DIRTY_GET ||
				op->type == KeyspaceOp::ADD ||
				op->type == KeyspaceOp::RENAME)
			{
				if (op->status)
					resp.Ok(op->cmdID, op->value);
				else
					resp.Failed(op->cmdID);

				resp.Write(data);
			}
			else if (op->type == KeyspaceOp::SET ||
					 op->type == KeyspaceOp::TEST_AND_SET)
			{
				if (op->status)
					resp.Ok(op->cmdID);
				else
					resp.Failed(op->cmdID);

				resp.Write(data);
			}
			else if (op->type == KeyspaceOp::DELETE || op->type == KeyspaceOp::PRUNE)
			{
				if (op->status)
					resp.Ok(op->cmdID);
				else
					resp.Failed(op->cmdID);

				resp.Write(data);
			}
			else if (op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST)
			{
				if (op->key.length > 0)
				{
					resp.ListItem(op->cmdID, op->key);
					resp.Write(data);
				}
			}
			else if (op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP)
			{
				if (op->key.length > 0)
				{
					resp.ListPItem(op->cmdID, op->key, op->value);
					resp.Write(data);
				}
			}
			else
				ASSERT_FAIL();
			
			if (data.length > 0)
			{
				//Log_Trace("=== Sending to client: %.*s ===", data.length, data.buffer);
				Write(data);
			}
			
			if (final && 
				(op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST ||
				 op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP))
			{
				resp.ListEnd(op->cmdID);
				resp.Write(data);
				//Log_Trace("=== Sending to client: %.*s ===", data.length, data.buffer);
				Write(data);
			}
		}
	}
	
	if (final)
	{
		numpending--;
		delete op;
	}

	if (state == DISCONNECTED && numpending == 0)
		server->DeleteConn(this);
}

void KeyspaceConn::OnMessageRead(const ByteString& message)
{
	req.Init();
	if (req.Read(message))
		ProcessMsg();
	else
		OnClose();
}

void KeyspaceConn::Write(ByteString &bs)
{
	static ByteArray<64> prefix;
	
	prefix.length = snwritef(prefix.buffer, prefix.size, "%d:", bs.length);

	TCPConn<>::Write(prefix.buffer, prefix.length, false);
	TCPConn<>::Write(bs.buffer, bs.length);
}

void KeyspaceConn::ProcessMsg()
{
//	Log_Trace();

	static ByteArray<32> ba;
	
	if (req.type == KEYSPACECLIENT_GET_MASTER)
	{
		int master = kdb->GetMaster();
		if (master < 0)
			resp.Failed(req.cmdID);
		else
		{
			ba.length = snwritef(ba.buffer, ba.size, "%d", master);
			resp.Ok(req.cmdID, ba);
		}
		resp.Write(data);
		Write(data);
		return;
	}
	else if (req.type == KEYSPACECLIENT_SUBMIT)
	{
		kdb->Submit();
		
		Log_Trace("numpending: %d", numpending);
		
		return;
	}
	
	KeyspaceOp* op;
	
	op = new KeyspaceOp;
	op->service = this;
	
	if (!req.ToKeyspaceOp(op))
	{
		delete op;
		OnClose();
		return;
	}
	
	if (!Add(op))
	{
		delete op;
		resp.Failed(op->cmdID);
		resp.Write(data);
		Write(data);
		closeAfterSend = true;
		return;
	}
}

void KeyspaceConn::OnClose()
{
	Log_Trace("numpending: %d", numpending);
	
//	if (connectionTimeout.IsActive())
//		EventLoop::Remove(&connectionTimeout);
	
	Close();
	if (numpending == 0)
		server->DeleteConn(this);
}

void KeyspaceConn::OnWrite()
{
	Log_Trace();

//	if (connectionTimeout.IsActive())
//		EventLoop::Reset(&connectionTimeout);
	
	TCPConn<>::OnWrite();
	if (closeAfterSend && !tcpwrite.active)
		OnClose();
}

void KeyspaceConn::OnConnectionTimeout()
{
	Log_Trace("numpending: %d", numpending);
	
	OnClose();
}
