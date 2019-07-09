--- net_utils.h.orig	2019-06-17 13:14:46.407629000 +0300
+++ net_utils.h	2019-07-09 11:47:47.757560000 +0300
@@ -28,10 +28,62 @@
 #ifndef _NET_UTILS_H_
 #define _NET_UTILS_H_
 
+#include <sys/param.h>
+#include <unistd.h>
+#include <time.h>
+
 #include <stdint.h>
 #include "pci_emul.h"
 
+struct token_bucket {
+	uint64_t tokens;
+	uint64_t last_ts;
+	uint64_t rate;
+};
+
 void	net_genmac(struct pci_devinst *pi, uint8_t *macaddr);
 int	net_parsemac(char *mac_str, uint8_t *mac_addr);
+
+__inline void
+token_bucket_rate_limit(struct token_bucket *tb, unsigned int n)
+{
+	struct timespec ts;
+	int64_t need;
+	uint64_t tokens;
+	uint64_t wait_us;
+	uint64_t burst;
+
+	if (tb->rate == 0)
+		return;
+
+	/* Convert packet len to bits. */
+	n = n << 3;
+
+	/* Set burst size */
+	burst = tb->rate * 10;
+
+	if (tb->tokens < n) {
+		clock_gettime(CLOCK_MONOTONIC_PRECISE, &ts);
+		uint64_t now = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
+
+		if (now > tb->last_ts) {
+			tokens = tb->tokens + tb->rate * (now - tb->last_ts);
+			tb->tokens += MIN(tokens, burst);
+			tb->last_ts = now;
+		}
+
+		if (tb->tokens < n) {
+			need = n - tb->tokens;
+			wait_us = (need / tb->rate + 1);
+
+			usleep(wait_us);
+
+			tb->tokens += need;
+			tb->last_ts = now + wait_us;
+		}
+	}
+
+	tb->tokens -= n;
+}
 
 #endif /* _NET_UTILS_H_ */
