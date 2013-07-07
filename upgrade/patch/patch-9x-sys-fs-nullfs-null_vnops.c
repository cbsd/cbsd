--- null_vnops.c-orig	2013-07-05 12:51:50.170412617 +0400
+++ null_vnops.c	2013-07-05 12:53:06.219412883 +0400
@@ -554,6 +554,7 @@
 	struct vnode *fvp = ap->a_fvp;
 	struct vnode *fdvp = ap->a_fdvp;
 	struct vnode *tvp = ap->a_tvp;
+	struct null_node *tnn;
 
 	/* Check for cross-device rename. */
 	if ((fvp->v_mount != tdvp->v_mount) ||
@@ -568,7 +569,11 @@
 		vrele(fvp);
 		return (EXDEV);
 	}
-	
+
+	if (tvp != NULL) {
+	    tnn = VTONULL(tvp);
+	    tnn->null_flags |= NULLV_DROP;
+	}	
 	return (null_bypass((struct vop_generic_args *)ap));
 }
 
