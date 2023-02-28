// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    // On page-load AJAX Example
    $.ajax({
        type: 'get',            //Request type
        dataType: 'json',       //Data type - we will use JSON for almost everything
        url: '/someendpoint',   //The server endpoint we are connecting to
        success: function (data) {
            /*  Do something with returned object
                Note that what we get is an object, not a string,
                so we do not need to parse it on the server.
                JavaScript really does handle JSONs seamlessly
            */

            //We write the object to the console to show that the request was successful
            console.log(data);
        },
        fail: function(error) {
            // Non-200 return, do something with error
            console.log(error);
        }
    });

    // Event listener form replacement example, building a Single-Page-App, no redirects if possible
    $('#someForm').submit(function(e){
		createGEDCOM();
        e.preventDefault();
        $.ajax({
			success: function(data) {
				alert(data)
			}
		});
    });
});

$(function(){
    $("#upload_link").on('click', function(e){
        e.preventDefault();
        $("#upload:hidden").trigger('click');
    });
});

function createGEDCOM() {
	var form = document.getElementById('create_gedcom');
	var file = form.c_file.value;
	var src = form.c_src.value;
	var enc = form.c_enc.value;
	var vers = form.c_vers.value;
	var s_name = form.c_s_name.value;
	var s_addr = form.c_s_addr.value;

	var table = document.getElementById('objects');
	var orig_row = document.getElementById('template');
	var clone = orig_row.cloneNode(true);

	clone.cells[0].innerHTML =  '<div class=scrollable><a href=\"\" onclick=\"changeStatus(\'status\', \'shakespeare.ged downloaded\');return false;\">' + file + '<br></a></div>';
	clone.cells[1].innerHTML = src;
	clone.cells[2].innerHTML = vers;
	clone.cells[3].innerHTML = enc;
	clone.cells[4].innerHTML = s_name;
	clone.cells[5].innerHTML = s_addr;
	clone.cells[6].innerHTML = '1';
	clone.cells[7].innerHTML = '1';

	table.appendChild(clone);

}

function addIndividual() {
	var form = document.getElementById('create_individual');
	var select = document.getElementById('add_ind_file');
	var fname = form.f_name.value;
	var lname = form.l_name.value;
	var file = select.options[select.selectedIndex].value;
	var status = document.getElementById('status').innerHTML;
	var new_status = ('Added individual ' + fname + ' ' + lname + ' to file ' + file + '<br>' + status);
	document.getElementById('status').innerHTML = new_status;
}

function getDescendants() {
	var div = document.getElementById('descendants');
	var table = document.getElementById('indivs');
	var clone = table.cloneNode(true);
	div.appendChild(clone);
}

function getAncestors() {
	var div = document.getElementById('ancestors');
	var table = document.getElementById('indivs');
	var clone = table.cloneNode(true);
	div.appendChild(clone);
}

function changeStatus(elem_id) {
	if (elem_id === 'status') {
		var status = document.getElementById('status').innerHTML;
		var new_status = ('shakespeare.ged downloaded' + '<br>' + status);
		document.getElementById('status').innerHTML = new_status;
	}
	if (elem_id === 'upload') {
		var name = document.getElementById('upload');
		var file = name.files.item(0).name;

		var status = document.getElementById('status').innerHTML;
		var new_status = 'Uploaded ' + file + '<br>' + status;
		document.getElementById('status').innerHTML = new_status;

		var table = document.getElementById('objects');
		var orig_row = document.getElementById('template');
		var clone = orig_row.cloneNode(true);

		clone.cells[0].innerHTML =  '<div class=scrollable><a href=\"\" onclick=\"changeStatus(\'status\', \'shakespeare.ged downloaded\');return false;\">' + file + '<br></a></div>';
		for (var i = 1; i < clone.cells.length; i++) {
			clone.cells[i].innerHTML = 'dummy';
		}
		table.appendChild(clone);
	}
};
