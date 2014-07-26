--- node-v0.10.29/src/node_win32_etw_provider-inl.h.orig	2014-06-09 18:04:36.000000000 +0100
+++ node-v0.10.29/src/node_win32_etw_provider-inl.h	2014-07-26 13:33:01.848026700 +0100
@@ -25,6 +25,14 @@
 #include "node_win32_etw_provider.h"
 #include "node_etw_provider.h"
 
+#if defined(_WIN64)
+# define INTPTR INT64
+# define ETW_WRITE_INTPTR_DATA ETW_WRITE_INT64_DATA
+#else
+# define INTPTR INT32
+# define ETW_WRITE_INTPTR_DATA ETW_WRITE_INT32_DATA
+#endif
+
 namespace node {
 
 using namespace v8;
@@ -94,7 +102,7 @@
     ETW_WRITE_ADDRESS_DATA(descriptors, &context);                            \
     ETW_WRITE_ADDRESS_DATA(descriptors + 1, &startAddr);                      \
     ETW_WRITE_INT64_DATA(descriptors + 2, &size);                             \
-    ETW_WRITE_INT32_DATA(descriptors + 3, &id);                               \
+    ETW_WRITE_INTPTR_DATA(descriptors + 3, &id);                              \
     ETW_WRITE_INT16_DATA(descriptors + 4, &flags);                            \
     ETW_WRITE_INT16_DATA(descriptors + 5, &rangeId);                          \
     ETW_WRITE_INT64_DATA(descriptors + 6, &sourceId);                         \
@@ -233,7 +241,7 @@
     }
     void* context = NULL;
     INT64 size = (INT64)len;
-    INT32 id = (INT32)addr1;
+    INTPTR id = (INTPTR)addr1;
     INT16 flags = 0;
     INT16 rangeid = 1;
     INT32 col = 1;
