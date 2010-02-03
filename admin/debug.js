
var debug = {
	contentDiv: null,
	autoFollow: false,
	autoTrace: false,
	newline: "<br/>",
	space: "&nbsp;",
	cmdHistory: [""],
	historyPos: 0,

	init: function()
	{		
		if (this.contentDiv != null)
			return;

		var debugDiv = util.elem("debug");
		if (debugDiv == undefined)
			return;

		if (this.contentDiv == null)
		{
			var children = debugDiv.getElementsByTagName("div");
			for (var key in children)
			{
				var node = children[key];
				if (node.id == "content")
				{
					this.contentDiv = node;
					break;
				}
			}
		}
	},

	toggleTrace: function()
	{
		var debugDiv = util.elem("debug");
		if (debugDiv.style.visibility == "hidden" || debugDiv.style.visibility == "")
		{
			debugDiv.style.visibility = "visible";
			var debugForm = util.elem("debug_form");
			debugForm.expr.focus();
		}
		else
			debugDiv.style.visibility = "hidden";

		return true;
	},

	clearTrace: function()
	{
		if (this.contentDiv)
			this.contentDiv.innerHTML = "";				
	},

	evalExpression: function()
	{
		var debugForm = util.elem("debug_form");
		var cmd = debugForm.expr.value;
		this.cmdHistory.pop()
		this.cmdHistory.push(cmd);
		this.cmdHistory.push("");
		this.historyPos = this.cmdHistory.length - 1;
		
		// aliases
		switch (cmd)
		{
		case "cls":
			cmd = "debug.clearTrace()";
			break;
		case "hide":
			cmd = "debug.toggleTrace()";
			break;
		}
		
		util.trace("executing '" + cmd + "'");
		this.scrollDown();
		eval(cmd);
		debugForm.expr.value = "";
	},
	
	onKeyUp: function(event)
	{
		var evt = window.event || event;
		var debugForm = util.elem("debug_form");
		switch (evt.keyCode)
		{
		case 38: // arrow-up
			if (this.historyPos > 0)
				this.historyPos--;
			if (this.cmdHistory.length > this.historyPos)
				debugForm.expr.value = this.cmdHistory[this.historyPos];
			if (evt.preventDefault)
				evt.preventDefault();
			else
				return false; // suppress default action
			break;
		case 40: // arrow-down
			if (this.historyPos < this.cmdHistory.length - 1)
				this.historyPos++;
			if (this.cmdHistory.length > this.historyPos)
				debugForm.expr.value = this.cmdHistory[this.historyPos];
			if (evt.preventDefault)
				evt.preventDefault();
			else
				return false; // suppress default action
			break;
		case 190:
			// TODO this does not work in FF	
			if (!this.autoTrace)
				break;

			var expr = debugForm.expr.value;
			if (expr.lastIndexOf('.') == debugForm.expr.value.length - 1)
			{
				var debugObj = expr.substring(0, expr.length - 1);
				var cmd = "debug.obj = " + debugObj;
				eval(cmd);				
				this.showObject(this.obj, debugObj);
			}
			break;
		}
		
		return true;
	},
	
	showObject: function(obj, name)
	{
		var value = "";
		if (typeof obj == "object")
		{
			value += "{" + this.newline;
			for (var m in obj)
			{
				var type = typeof obj[m];
				value += this.space + m + ": " + type;
				if (type != "object" && type != "function")
					value += " [" + obj[m] + "]";
				value += "," +  "\n";
			}
			value += "}" + "\n";
			
		}
		else
			value += "" + obj + "\n";
	
		util.trace(this.newline + name + ": " + value);
	},
	
	scrollDown: function()
	{
		this.contentDiv.scrollTop = this.contentDiv.scrollHeight;
	},
	
	move: function(x, y)
	{
		var debugDiv = util.elem("debug");
		debugDiv.style.left = x;
		debugDiv.style.top = y;
	},
	
	resize: function(w, h)
	{
		var debugDiv = e("debug");
		debugDiv.style.width = w;
		debugDiv.style.height = h;
	},
	
	trace: function(msg)
	{
		msg = msg.replace(/\n/g, this.newline);
		msg = msg.replace(/\t/g, this.space + this.space);
		this.init();
		if (this.contentDiv)
		{
			// var caller = this.trace.caller.toString();
			var caller = "";
			this.contentDiv.innerHTML += this.longString() + ": " + caller + ": " + msg + "<br/>";
			if (this.autoFollow)
				this.scrollDown();
		}
	},
	
	longString: function(d)
	{
		if (d == undefined || d == null)
			d = new Date();
		var str = util.padString(d.getYear() + 1900, 4, "0") +
					"-" +
					util.padString(d.getMonth() + 1, 2, "0") +
					"-" +
					util.padString(d.getDate(), 2, "0") +
					" " +
					util.padString(d.getHours(), 2, "0") + 
					":" +
					util.padString(d.getMinutes(), 2, "0") +
					":" +
					util.padString(d.getSeconds(), 2, "0") +
					"." +
					util.padString(d.getMilliseconds(), 3, "0");
		return str;
	},
	
	loadScript: function(js)
	{
		var scriptTag = document.createElement("script");
		scriptTag.setAttribute("type", "text/javascript");
		scriptTag.setAttribute("src", js);
		document.getElementsByTagName("head")[0].appendChild(scriptTag);		
	},
	
	loadStyle: function(css)
	{
		var scriptTag = document.createElement("link");
		scriptTag.setAttribute("type", "text/css");
		scriptTag.setAttribute("rel", "stylesheet");
		scriptTag.setAttribute("href", css);
		document.getElementsByTagName("head")[0].appendChild(scriptTag);				
	},
	
	loadjQuery: function()
	{
		this.loadStyle("http://jqueryui.com/latest/themes/base/ui.all.css");
		this.loadScript("http://jqueryjs.googlecode.com/files/jquery-1.3.2.min.js");
		this.loadScript("http://ajax.googleapis.com/ajax/libs/jqueryui/1.7.2/jquery-ui.min.js");
	},
	
	initjQuery: function()
	{
		$("#debug").resizable();
		$("#debug").draggable();
	},
	
	pieDemo: function()
	{
		var scriptTag = document.createElement("div");
		scriptTag.setAttribute("id", "holder");
		util.elem("debug").appendChild(scriptTag);
		this.loadScript("http://raphaeljs.com/raphael.js");
		this.loadScript("js/pie.js");
	}
}


