//JavaScript Document 

var cname='BoxOn';

var cookies = document.cookie;

var pos1 = cookies.indexOf(escape(cname) + '=');

	if (pos1 != -1)
	{
		pos1 = pos1 + (escape(cname) + '=').length;
		pos2 = cookies.indexOf(';', pos1);
		if (pos2 == -1) pos2 = cookies.length;

		hodnota = cookies.substring(pos1, pos2);

		if (hodnota == "on")
		{
			$("div#hidden_box").show();
			
		}
		else
		{
			$("div#hidden_box").hide();
		}
	}
	else
	{
		$("div#hidden_box").hide();
	}

$("img#more").click(function() 
{
	cookies = document.cookie;
	pos1 = cookies.indexOf(escape(cname) + '=');

	if (pos1 != -1)
	{
		//hledej konec hodnoty
		pos1 = pos1 + (escape(cname) + '=').length;
		pos2 = cookies.indexOf(';', pos1);
		if (pos2 == -1) pos2 = cookies.length;

		//nastav styl na hodnotu
		hodnota = cookies.substring(pos1, pos2);

		if (hodnota == "on")
		{
			$("div#hidden_box").hide("slow");
			document.cookie = "BoxOn=off;";
		}
		else
		{
			$("div#hidden_box").show("slow");
			document.cookie = "BoxOn=on;";
		}
	}

	else
	{
		$("div#hidden_box").show("slow");
		document.cookie = "BoxOn=on;";
	}
});


