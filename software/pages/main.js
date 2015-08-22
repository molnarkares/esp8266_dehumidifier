$(document).ready(function(){
	processJson();
	addnotify();
	});

var jsonLoaded = false;

function processJson()
{
	$.getJSON("./list.json",function(c) 
	{
		var d=[];
		var changed = false;
		d.push('<li><a>'
		+'<h3>Temperature</h3>'
		+'<p class=\"ui-li-aside\"><sup>'+c.temp.toFixed(1)+'&deg;C</sup></p></a></li>'
		+'<li><a>'
		+'<h3>Humidity</h3>'
		+'<p class=\"ui-li-aside\"><sup>'+c.hum.toFixed(1)+'&#37</sup></p>'
		+'</a></li>');
		$('#list').html(d.join('')).trigger('create').listview('refresh');
		$('#info').html('Uptime: '+toTime(c.uptime)+' ('+c.free+' bytes free)');
		$('#autocontrol').val(c.auto).flipswitch( "refresh" );
		$('#mancontrol').val(c.manual).flipswitch( "refresh" );
		$('#water-container').val(c.full).flipswitch( "refresh" );
		$('#humset').val(c.desired).slider("refresh");
		jsonLoaded = true;
	});
	setTimeout(processJson,5000);
}

function addnotify() {
	this.notify = function() {
		if(!jsonLoaded) return;
		$.post("control.php",
        {
          manual: 	$('#mancontrol').val(),
		  auto: 	$('#autocontrol').val(),
          desired:	$('#humset').val()
        }/*,function( data ) {
			alert( "DATA notify: " + data );
		}*/);
	};
	$('#autocontrol').change(function() {
		if($('#autocontrol').val() === 'on') {
			$('#mancontrol').flipswitch('disable');
			$('#humset').slider('enable');
		}else {
			$('#mancontrol').flipswitch('enable');
			$('#humset').slider('disable');
		}
		notify();
	});
	$('#mancontrol').change(notify);
	$( "#humset" ).on( "slidestop", notify);	
}

function toTime(sx) {
	var x = parseInt(sx);
	var  retval = "";
	this.toStr = function (y) {
		var ret = "";
		if (y <10) {
			ret = "0";
		}
		return (ret + y.toString());
	}
	var ms = x%1000;
	x-=ms;
	x /= 1000;
	var sec = x%60;
	x-=sec;
	x/=60;
	var min = x%60;
	x-=min;
	var hour = x/=60;
	retval =  toStr(hour)+":"+toStr(min)+":"+toStr(sec);
	return retval;
}
