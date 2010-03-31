require 'keyspace_client'

class Keyspace
	class Result
		def initialize(cptr)
			@keys = []
			@values = []
			@cmd_statuses = []
			@transport_status = Keyspace_client.Keyspace_ResultTransportStatus(cptr)
			@connectivity_status = Keyspace_client.Keyspace_ResultConnectivityStatus(cptr)
			@timeout_status = Keyspace_client.Keyspace_ResultTimeoutStatus(cptr)
			@cursor = 0
			@num_elem = 0
			while not Keyspace_client.Keyspace_ResultIsEnd(cptr)
				@keys << Keyspace_client.Keyspace_ResultKey(cptr)
				@values << Keyspace_client.Keyspace_ResultValue(cptr)
				@cmd_statuses << Keyspace_client.Keyspace_ResultCommandStatus(cptr)
				@num_elem += 1
				Keyspace_client.Keyspace_ResultNext(cptr)
			end
			Keyspace_client.Keyspace_ResultClose(cptr)
		end

		public

		attr_reader :keys
	
		attr_reader :transport_status
		attr_reader :connectivity_status
		attr_reader :timeout_status

		def command_status
			return @cmd_status[@cursor]
		end

		def key
			return @keys[@cursor]
		end

		def value
			return @values[@cursor]
		end

		def begin
			@cursor = 0
		end

		def is_end?
			return true if @cursor == @num_elem
			return false
		end

		def next
			if @cursor < @num_elem
				@cursor += 1
			end
		end

		def key_values
			kv = {}
			self.begin
			while not self.is_end?
				kv[self.key] = self.value
				self.next
			end
			return kv
		end
		
	end

	def initialize(nodes)
		@cptr = Keyspace_client.Keyspace_Create()
		@result = nil
		node_params = Keyspace_client::Keyspace_NodeParams.new(nodes.length)
		nodes.each do |node|
			node_params.AddNode(node)
		end
		Keyspace_client.Keyspace_Init(@cptr, node_params)
		node_params.Close()
	end

	public

	attr_reader :result

	def set_global_timeout(timeout)
		Keyspace_client.Keyspace_SetGlobalTimeout(@cptr, timeout)
	end

	def get_global_timeout
		return Keyspace_client.Keyspace_GetGlobalTimeout(@cptr)
	end

	def set_master_timeout(timeout)
		Keyspace_client.Keyspace_SetMasterTimeout(@cptr, timeout)
	end

	def get_master_timeout
		return Keyspace_client.Keyspace_GetMasterTimeout(@cptr)
	end

	def get_master
		return Keyspace_client.Keyspace_GetMaster(@cptr)
	end

	def get_state
		return Keyspace_client.Keyspace_GetState(@cptr)
	end

	def distribute_dirty(dd)
		return Keyspace_client.Keyspace_DistributeDirty(dd)
	end

	def get(key)
		status = Keyspace_client.Keyspace_Get(@cptr, key)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end

		return nil if Keyspace_client.Keyspace_IsBatched(@cptr)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return @result.value			
	end

	def dirty_get(key)
		status = Keyspace_client.Keyspace_DirtyGet(@cptr, key)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end

		return nil if Keyspace_client.Keyspace_IsBatched(@cptr)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return @result.value			
	end

	def count_(prefix = "", start_key = "", count = 0, skip = false, forward = true)
		status = Keyspace_client.Keyspace_Count(@cptr, prefix, start_key, count, skip, forward)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return nil if status < 0
		return @result.value
	end

	def count(args = {})
		prefix = self.listArg(args, "prefix")
		start_key = self.listArg(args, "start_key")
		count = self.listArg(args, "count")
		skip = self.listArg(args, "skip")
		forward = self.listArg(args, "forward")
		return self.count_(prefix, start_key, count, skip, forward)
	end

	def dirty_count_(prefix = "", start_key = "", count = 0, skip = false, forward = true)
		status = Keyspace_client.Keyspace_DirtyCount(@cptr, prefix, start_key, count, skip, forward)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return nil if status < 0
		return @result.value
	end

	def dirty_count(args = {})
		prefix = self.listArg(args, "prefix")
		start_key = self.listArg(args, "start_key")
		count = self.listArg(args, "count")
		skip = self.listArg(args, "skip")
		forward = self.listArg(args, "forward")
		return self.dirty_count_(prefix, start_key, count, skip, forward)
	end

	def list_keys_(prefix = "", start_key = "", count = 0, skip = false, forward = true)
		status = Keyspace_client.Keyspace_ListKeys(@cptr, prefix, start_key, count, skip, forward)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return nil if status < 0
		return @result.keys
	end

	def list_keys(args = {})
		prefix = self.listArg(args, "prefix")
		start_key = self.listArg(args, "start_key")
		count = self.listArg(args, "count")
		skip = self.listArg(args, "skip")
		forward = self.listArg(args, "forward")
		return self.list_keys_(prefix, start_key, count, skip, forward)	
	end

	def dirty_list_keys_(prefix = "", start_key = "", count = 0, skip = false, forward = true)
		status = Keyspace_client.Keyspace_DirtyListKeys(@cptr, prefix, start_key, count, skip, forward)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return nil if status < 0
		return @result.keys
	end

	def dirty_list_keys(args = {})
		prefix = self.listArg(args, "prefix")
		start_key = self.listArg(args, "start_key")
		count = self.listArg(args, "count")
		skip = self.listArg(args, "skip")
		forward = self.listArg(args, "forward")
		return self.dirty_list_keys_(prefix, start_key, count, skip, forward)	
	end

	def list_key_values_(prefix = "", start_key = "", count = 0, skip = false, forward = true)
		status = Keyspace_client.Keyspace_ListKeyValues(@cptr, prefix, start_key, count, skip, forward)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return nil if status < 0
		return @result.key_values
	end

	def list_key_values(args = {})
		prefix = self.listArg(args, "prefix")
		start_key = self.listArg(args, "start_key")
		count = self.listArg(args, "count")
		skip = self.listArg(args, "skip")
		forward = self.listArg(args, "forward")
		return self.list_key_values_(prefix, start_key, count, skip, forward)	
	end

	def dirty_list_key_values_(prefix = "", start_key = "", count = 0, skip = false, forward = true)
		status = Keyspace_client.Keyspace_DirtyListKeyValues(@cptr, prefix, start_key, count, skip, forward)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return nil if status < 0
		return @result.key_values
	end

	def dirty_list_key_values(args = {})
		prefix = self.listArg(args, "prefix")
		start_key = self.listArg(args, "start_key")
		count = self.listArg(args, "count")
		skip = self.listArg(args, "skip")
		forward = self.listArg(args, "forward")
		return self.dirty_list_key_values_(prefix, start_key, count, skip, forward)	
	end

	def set(key, value)
		status = Keyspace_client.Keyspace_Set(@cptr, key, value)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end
		return nil if is_batched?
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
	end

	def test_and_set(key, test, value)
		status = Keyspace_client.Keyspace_TestAndSet(@cptr, key, test, value)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end
		return nil if is_batched?
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return @result.value
	end

	def add(key, num)
		status = Keyspace_client.Keyspace_Add(@cptr, key, num)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end
		return nil if is_batched?
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return @result.value
	end

	def delete(key)
		status = Keyspace_client.Keyspace_Delete(@cptr, key)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end
		return nil if is_batched?
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
	end

	def remove(key)
		status = Keyspace_client.Keyspace_Remove(@cptr, key)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end
		return nil if is_batched?
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return @result.value
	end

	def rename(src, dst)
		status = Keyspace_client.Keyspace_Rename(@cptr, src, dst)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end
		return nil if is_batched?
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
	end

	def prune(prefix)
		status = Keyspace_client.Keyspace_Prune(@cptr, prefix)
		if status < 0
			@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
			return nil
		end
		return nil if is_batched?
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
	end

	def begin
		return Keyspace_client.Keyspace_Begin(@cptr)
	end

	def submit
		status = Keyspace_client.Keyspace_Submit(@cptr)
		@result = Result.new(Keyspace_client.Keyspace_GetResult(@cptr))
		return status
	end

	def cancel
		return Keyspace_client.Keyspace_Cancel(@cptr)
	end

	def is_batched?
		return Keyspace_client.Keyspace_IsBatched(@cptr)
	end

	def listArg(args, name)
		return args[name] if args.has_key?(name)
		result = case name
			when "prefix" then ""
			when "start_key" then ""
			when "count" then 0
			when "skip" then false
			when "forward" then true
			else nil
		end
		return result
	end
end
