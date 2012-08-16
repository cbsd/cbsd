<?php
?>
<script language="JavaScript1.2" type="text/javascript">
<!--
	// Checkboxes
	function Toggle(e) {
		if(e.checked) {
			Highlight(e);
			document.selform.toggleAllC.checked = AllChecked();
		} else {
			UnHighlight(e);
			document.selform.toggleAllC.checked = false;
		}
   	}

	function ToggleAll(e) {
		if(e.checked) CheckAll();
		else ClearAll();
	}
	
	function CheckAll() {
		var ml = document.selform;
		var len = ml.elements.length;
		for(var i=0; i<len; ++i) {
			var e = ml.elements[i];
			if(e.name == "selitems[]") {
				e.checked = true;
				Highlight(e);
			}
		}
		ml.toggleAllC.checked = true;
	}

	function ClearAll() {
		var ml = document.selform;
		var len = ml.elements.length;
		for (var i=0; i<len; ++i) {
			var e = ml.elements[i];
			if(e.name == "selitems[]") {
				e.checked = false;
				UnHighlight(e);
			}
		}
		ml.toggleAllC.checked = false;
	}
   
	function AllChecked() {
		ml = document.selform;
		len = ml.elements.length;
		for(var i=0; i<len; ++i) {
			if(ml.elements[i].name == "selitems[]" && !ml.elements[i].checked) return false;
		}
		return true;
	}
	
	function NumChecked() {
		ml = document.selform;
		len = ml.elements.length;
		num = 0;
		for(var i=0; i<len; ++i) {
			if(ml.elements[i].name == "selitems[]" && ml.elements[i].checked) ++num;
		}
		return num;
	}
	
	
	// Row highlight

	function Highlight(e) {
		var r = null;
		if(e.parentNode && e.parentNode.parentNode) {
			r = e.parentNode.parentNode;
		} else if(e.parentElement && e.parentElement.parentElement) {
			r = e.parentElement.parentElement;
		}
		if(r && r.className=="rowdata") {
			r.className = "rowdatasel";
		}
	}

	function UnHighlight(e) {
		var r = null;
		if(e.parentNode && e.parentNode.parentNode) {
			r = e.parentNode.parentNode;
		} else if (e.parentElement && e.parentElement.parentElement) {
			r = e.parentElement.parentElement;
		}
		if(r && r.className=="rowdatasel") {
			r.className = "rowdata";
		}
	}
	
<?php if($allow) { ?>
	
	// Copy / Move / Delete
	
	function Copy() {
		if(NumChecked()==0) {
			alert("<?php echo $GLOBALS["error_msg"]["miscselitems"]; ?>");
			return;
		}
		document.selform.do_action.value = "copy";
		document.selform.submit();
	}
	
	function Move() {
		if(NumChecked()==0) {
			alert("<?php echo $GLOBALS["error_msg"]["miscselitems"]; ?>");
			return;
		}
		document.selform.do_action.value = "move";
		document.selform.submit();
	}
	
	function Chmod() {
		if(NumChecked()==0) {
			alert("<?php echo $GLOBALS["error_msg"]["miscselitems"]; ?>");
			return;
		}
		document.selform.do_action.value = "chmod";
		document.selform.submit();
	}
	
	function Delete() {
		num=NumChecked();
		if(num==0) {
			alert("<?php echo $GLOBALS["error_msg"]["miscselitems"]; ?>");
			return;
		}
		if(confirm("<?php echo $GLOBALS["error_msg"]["miscdelitems"]; ?>")) {
			document.selform.do_action.value = "delete";
			document.selform.submit();
		}
	}
	
	function Archive() {
		if(NumChecked()==0) {
			alert("<?php echo $GLOBALS["error_msg"]["miscselitems"]; ?>");
			return;
		}
		document.selform.do_action.value = "arch";
		document.selform.submit();
	}
	
<?php } ?>

// -->
</script>
