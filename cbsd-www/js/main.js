var jail=
{
	action:function(jname,cmd)
	{
		alert(cmd+' jail: '+jname);
		data.post({},this.actionOk);
	},
	actionOk:function(data)
	{
		alert(data.msg);
	},
	
	
	
};

var data=
{
	uri:'/ajax.php',
	
	post:function(data,callback)
	{
		if(typeof data != 'object')
		{
			alert('data must be array!');
			return false;
		};
		
		$.post(this.uri,function(data){
			callback(data);
		},'json');
	},
	
};