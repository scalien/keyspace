var keyspace = 
{       
    adminUrl: "http://localhost:8080/api",
	dbUrl: "http://localhost:8080/json",
	separator: "/",
	rowsPerPage: 10,
	debug: true,

	admin:
	{
		status: function()
		{
			json.rpc(keyspace.adminUrl, keyspace.printKeyValue, "status")
		},
	
		logtrace: function()
		{
			json.rpc(keyspace.adminUrl, keyspace.printData, "logtrace")
		},
	
		restart: function()
		{
			json.rpc(keyspace.adminUrl, keyspace.printData, "restart")
		}	
	},
	
	db:
	{
		// TODO getmaster is missing!! remove it from documentation
		getmaster: function()
		{
			json.rpc(keyspace.dbUrl, keyspace.printData, "getmaster");
		},
		
		get: function(key)
		{
			json.rpc(keyspace.dbUrl, keyspace.printKeyValue, "get", key);
		},

		dirtyget: function(key)
		{
			json.rpc(keyspace.dbUrl, keyspace.printKeyValue, "dirtyget", key);
		},
		
		set: function(key, value)
		{
			json.rpc(keyspace.dbUrl, keyspace.printKeyValue, "set", key, value);
		},

		testandset: function(key, test, value)
		{
			json.rpc(keyspace.dbUrl, keyspace.printKeyValue, "testandset", key, test, value);
		},

		add: function(key, num)
		{
			json.rpc(keyspace.dbUrl, keyspace.printKeyValue, "add", key, num);
		},

		rename: function(key, newKey)
		{
			json.rpc(keyspace.dbUrl, keyspace.printData, "set", key, newKey);
		},

		delete_: function(key)
		{
			json.rpc(keyspace.dbUrl, keyspace.printData, "delete", key);
		},

		remove: function(key)
		{
			json.rpc(keyspace.dbUrl, keyspace.printData, "remove", key);
		},
		
		prune: function(prefix)
		{
			json.rpc(keyspace.dbUrl, keyspace.printData, "prune", prefix);
		},
		
		listkeys: function(prefix, startkey, count, next, forward)
		{
			this._list("listkeys", prefix, startkey, count, next, forward);
		},

		listkeyvalues: function(prefix, startkey, count, next, forward)
		{
			this._list("listkeyvalues", prefix, startkey, count, next, forward);
		},
		
		dirtylistkeys: function(prefix, startkey, count, next, forward)
		{
			this._list("dirtylistkeys", prefix, startkey, count, next, forward);
		},
		
		dirtylistkeyvalues: function(prefix, startkey, count, next, forward)
		{
			this._list("dirtylistkeyvalues", prefix, startkey, count, next, forward);
		},
		
		count: function(prefix, startkey, count, next, forward)
		{
			json.rpc(keyspace.dbUrl, keyspace.printKeyValue, "count", prefix, startkey, count, next, forward);
		},
		
		dirtycount: function(prefix, startkey, count, next, forward)
		{
			json.rpc(keyspace.dbUrl, keyspace.printKeyValue, "dirtycount", prefix, startkey, count, next, forward);
		},
		
		_list: function(funcname, prefix, startkey, count, next, forward)
		{
			var listCallback = function(data) { 
				keyspace.printPaginatedList(data, funcname, prefix, startkey, count, next, forward);
			}
			json.rpc(keyspace.dbUrl, listCallback, funcname, prefix, startkey, count, next, forward);			
		},
	},
                        
	print: function(s)
	{
		util.elem("right").innerHTML = s;		
	},
	
	printData: function(data)
	{
		keyspace.print(data.status);
	},

	printKeyValue: function(data)
	{
		var s = "<table width='100%'>";
		for (k in data)
		{
			s += "<tr><td>" + k + "</td><td>" + data[k] + "</td></tr>";
		    break;
		}
		s += "</table>";
		keyspace.print(s);
	},
	
	createSepFuncLink: function(funcName, args, input, numarg)
	{
 		return this.separateString(input, function(full, part) { 
						var link = keyspace.makeLink(function(s) { 
							var out = funcName;
							for (var i = 0; i < args.length; i++)
							{
								if (typeof(args[i]) == "undefined")
									break;

								if (i == 0)
									out += "(";
								else
									out += ",";

								// alert(args[i]);
								if (i == numarg)
								{
									out += json.encode(s);
								}
								else
								{
									out += json.encode(args[i]);
								}
							}
							out += ")";
							return out;
						}, full, part);
						return link;
					});
   	
	},

 	createFuncLink: function(funcName, args, linktext)
	{
		return this.makeLink(function(s) {
			var out = funcName;
			for (var i = 0; i < args.length; i++)
			{
				if (typeof(args[i]) == "undefined")
					break;

				if (i == 0)
					out += "(";
				else
					out += ",";

				out += json.encode(args[i]);
			}
			out += ")";
			return out;
		}, linktext, linktext);
	},
   
	makeLink: function(funcNameFunc, full, part)
	{
		var linkstr = funcNameFunc(full);
		var link = "<a href='#' onclick='" + linkstr + ";'>" + part + "</a>";
		return link;
	},
	
	separateString: function(s, partFunc)
	{
		if (!s)
			return "";

		var i = 0;
		var start = 0;
		var out = "";
		while (true)
		{
			i = s.indexOf(keyspace.separator, i);
			if (i == -1)
				break;
			if (i > 0)
				out += partFunc(s.substring(0, i), s.substring(start, i));
			out += keyspace.separator;
			i += 1;
			start = i;
		}
		out += s.substring(start);
		return out;
	},
	
	functionString: function()
	{
		// var args = arguments.callee.caller.arguments;
		var args = arguments;
		var fn = args[0];
		for (i = 1; i < args.length; i++)
		{
			if (i == 1)
				fn += "(";
			else
				fn += ","

			fn += args[i];
		}
		fn += ")";
		return fn;
	},
	
	safeArg: function(arg)
	{
		if (typeof(arg) != "undefined")
			return json.encode(arg);
		return "";
	},
	           
	generatePrevNext: function(func, last, prefix, startkey, count, next)
	{
		var s = "";
		s += "<br/>";
		if (typeof(next) == "undefined")
			next = 0;
		if (next >= count)
		{			
			s += "<a href='#' onclick='keyspace.db." + func + "(" + 
						json.encode(prefix) + "," +
						json.encode(startkey) + "," +
						json.encode(count) + "," +
						json.encode(next - count) +
					");'>prev</a> |";
		}
		else
			s += "prev |";

		if (last != "")
		{
			s += " <a href='#' onclick='keyspace.db." + func + "(" + 
						json.encode(prefix) + "," +
						json.encode(startkey) + "," +
						json.encode(count) + "," +
						json.encode(next + count) +
					");'>next</a>";


		}
		else
			s += "&nbsp;next";

		return s;
	},
	
	printPaginatedList: function(data, func, prefix, startkey, count, next, forward)
	{
		// util.trace(prefix);
		if (typeof(data[prefix]) == "array")
			var keys = data[prefix].sort();
		else
			var keys = util.keys(data[prefix]).sort();
		var first = keys[0];
		if (keys.length > 0)
			var last = keys[keys.length - 1];
		else
			var last = "";

		var nav = keyspace.generatePrevNext(func, last, prefix, startkey, count, next);
		var prefixLinked = "\"" + keyspace.createSepFuncLink("keyspace.db." + func, new Array(prefix, startkey, count, next, forward), prefix, 0) + "\"";
		var startkeyLinked = "\"" + keyspace.createSepFuncLink("keyspace.db." + func, new Array(prefix, startkey, count, next, forward), startkey, 1) + "\"";
		var s = "<h2>" + keyspace.functionString(func, prefixLinked, startkeyLinked, keyspace.safeArg(count), keyspace.safeArg(next), keyspace.safeArg(forward)) + "</h2>";		
		s += nav + "<br/>";
		s += this.createList(data, func, prefix, startkey, count, next, forward);
		s += nav;

		keyspace.print(s);
	},
		
	createList: function(data, func, prefix, startkey, count, next, forward)
	{
		var s = "<table width='100%'>";
		
		var even = true;
		for (var o in data)
		{
			var list = data[o];
			if (func == "listkeyvalues" || func == "dirtylistkeyvalues")
			{
				var keys = util.keys(list).sort();
				for (var i = 0; i < keys.length; i++)
				{
					var trclass = even ? "even" : "odd";
					// var skey = keyspace.separateString(keys[i], function(full, part) { return keyspace.makeLink(function(s) { return "keyspace.db." + func + "(\"" + s + "\",\"\"," + count + ")"}, full, part);});
					var skey = keyspace.createSepFuncLink("keyspace.db." + func, [prefix, "", count, 0, undefined], keys[i], 0);
					s += "<tr class='" + trclass + "'><td>" + skey + "</td><td>" + list[keys[i]] + "</td></tr>";
					even = !even;
				}
			}
			else
			{
				var keys = list.sort();
				for (var i = 0; i < keys.length; i++)
				{
					var trclass = even ? "even" : "odd";
					s += "<tr class='" + trclass + "'><td>" + this.createFuncLink("keyspace.db.get", [keys[i]], keys[i])  + "</td></tr>";
					even = !even;
				}				
			}
	 	}
	
		s += "</table>";	
		return s;
	},
	
	showLoading: function()
	{
		util.elem("loading").style.display = '';
	},

	hideLoading: function()
	{
	   	util.elem("loading").style.display = 'none';
	}
		
}	

var json =
{
	counter : 0,
	active : 0,
	func: {},

	get: function(url, userfunc, showload)
	{
		var id = json.counter++;

		url = url.replace(/USERFUNC/g, "json.func[" + id + "]");
		var scriptTag = document.createElement("script");
		scriptTag.setAttribute("id", "json" + id);
		scriptTag.setAttribute("type", "text/javascript");
		scriptTag.setAttribute("src", url);
		document.getElementsByTagName("head")[0].appendChild(scriptTag);
		if (showload)
			this.active++;
		util.trace("[" + json.active + "] calling " + url);

		this.func[id] = function(data)
		{
			if (data == undefined)
			{
				if (keyspace.debug)
					alert("json.func[" + id + "]: empty callback");
				util.trace("json.func[" + id + "]: empty callback");
				return;
			}

			if (data.hasOwnProperty("error"))
			{
				if (keyspace.debug)
					alert("json.func[" + id + "]: " + data.error);
				util.trace("json.func" + id + ": " + data.error);

				if (data.hasOwnProperty("type") && data.type == "session")
				{
					util.logout();
					return;
				}
			}

			util.trace("[" + json.active + "] json callback " + id);			
			userfunc(data);
			if (showload)
				json.active--;
			if (json.active == 0 && typeof(keyspace) != "undefined")
				keyspace.hideLoading();
			util.trace("[" + json.active + "] json callback " + id + " after userfunc");
			util.removeElement("json" + id);
			this.func = util.removeKey(this.func, "func" + id);
		}

		if (showload)
			keyspace.showLoading();
	},

	rpc: function(baseUrl, userfunc, cmd)
	{
		var url = baseUrl + ",USERFUNC/" + cmd ;
		var sep = "?";
		for (var i = 3; i < arguments.length; i++)
		{
			url += sep;
			if (sep == "?")
				sep = ","
		  	
			if (typeof(arguments[i]) != "undefined")
			{
				if (typeof(arguments[i]) == "object" || typeof(arguments[i]) == "array")
					var arg = JSON.stringify(arguments[i]);
				else
					var arg = arguments[i];
				var value = encodeURIComponent(arg);
			}
			else
				var value = "";
			url += value;
		}                                        

		json.get(url, userfunc, true);
	},
	
	encode: function(jsobj)
	{
		// if (typeof(jsobj) == "undefined")
		//  	return "";
		
		return JSON.stringify(jsobj);
	}
}

var util =
{
 	elem: function(id)
	{
		return document.getElementById(id);
	},

	keys: function(arr)
	{
		var r = new Array();
		for (var key in arr)
			r.push(key);
		return r;
	},
	
   removeKey: function(arr, key)
	{
		var n = new Array();
		for (var i in arr)
			if (i != key)
				n.push(arr[i]);	
		return n;
	},
	
	removeElement: function(id)
	{
		e = this.elem(id);
		e.parentNode.removeChild(e);
	},
	
	padString: function(str, width, pad)
	{
		str = "" + str;
		while (str.length < width)
			str = pad + str;
	
		return str;
	},

	escapeQuotes: function(str)
	{
		str = str.replace(/'/g, "\\x27");
		str = str.replace(/\"/g, "\\x22");
		return str;
	},
	
	trace: function(msg)
	{
		if (!keyspace.debug)
			return;

		debug.trace(msg);
	}
	
	
}