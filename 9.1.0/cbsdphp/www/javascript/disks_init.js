/*
	disks_init.js

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	Copyright (C) 2008 Volker Theile <votdev@gmx.de>
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met: 

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer. 
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies, 
	either expressed or implied, of the FreeBSD Project.
*/
function disk_change() {
	var devicespecialfile = document.iform.disk.value;
	x_get_fs_type(devicespecialfile, update_type);
}

function update_type(value) {
	for (i = 0; i < document.iform.type.length; i++) {
		document.iform.type.options[i].selected = false;
		if (document.iform.type.options[i].value == value) {
			document.iform.type.options[i].selected = true;
		}
	}
	type_change();
}

function type_change() {
	var value = document.iform.type.value;
	switch (value) {
		case "ufsgpt":
			showElementById('minspace_tr','show');
			showElementById('volumelabel_tr','show');
			showElementById('aft4k_tr','show');
			break;
		case "ext2":
		case "msdos":
			showElementById('minspace_tr','hide');
			showElementById('volumelabel_tr','show');
			showElementById('aft4k_tr','hide');
			break;
		default:
			showElementById('minspace_tr','hide');
			showElementById('volumelabel_tr','hide');
			showElementById('aft4k_tr','hide');
			break;
	}
}
