noinst_PROGRAMS = siltcluster master siltnode testSilt testFawnDS testIterator testTrie testCuckoo testCombi testByYCSBWorkload testByYCSBPUT testByYCSBTrans benchCuckoo benchSemiRandomWrite benchStores preprocessTrace
	
siltcluster_SOURCES = siltcluster.cpp
siltcluster_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

siltcluster_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(top_builddir)/siltds/libsiltcluster.la \
	-lthrift

siltnode_SOURCES = siltnode.cc csapp.c csapp.h
siltnode_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

siltnode_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(top_builddir)/siltds/libsiltcluster.la \
	$(THRIFT_LIBS)

master_SOURCES = master.cc master.h csapp.c csapp.h
master_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

master_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la 

testSilt_SOURCES = testSilt.cpp
testSilt_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

testSilt_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)

testFawnDS_SOURCES = testFawnDS.cc
testFawnDS_CPPFLAGS = 				\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

testFawnDS_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)

testIterator_SOURCES = testIterator.cc
testIterator_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

testIterator_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)

testTrie_SOURCES = testTrie.cc
testTrie_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

testTrie_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)

testCuckoo_SOURCES = testCuckoo.cc
testCuckoo_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

testCuckoo_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)

testCombi_SOURCES = testCombi.cc
testCombi_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

testCombi_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)


testByYCSBWorkload_SOURCES= testByYCSBWorkload.cc
testByYCSBWorkload_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_srcdir)/fawnkv 			\
	-I$(top_builddir)/fawnkv/gen-cpp 		\
	-I$(top_builddir)/fawnkv			\
	-I$(top_builddir)/fawnds		\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp \
	-I$(top_builddir)/test/siltds/gen-cpp

testByYCSBWorkload_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/fawnkv/libfawnkv.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(top_builddir)/siltds/libsiltcluster.la 	\
	-lthrift 			\
	$(THRIFT_LIBS)

testByYCSBTrans_SOURCES= testByYCSBTrans.cc
testByYCSBTrans_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_srcdir)/fawnkv 			\
	-I$(top_builddir)/fawnkv/gen-cpp 		\
	-I$(top_builddir)/fawnkv			\
	-I$(top_builddir)/fawnds		\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp \
	-I$(top_builddir)/test/siltds/gen-cpp

testByYCSBTrans_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/fawnkv/libfawnkv.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(top_builddir)/siltds/libsiltcluster.la 	\
	-lthrift 			\
	$(THRIFT_LIBS)


testByYCSBPUT_SOURCES= testByYCSBPUT.cc
testByYCSBPUT_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_srcdir)/fawnkv 			\
	-I$(top_builddir)/fawnkv/gen-cpp 		\
	-I$(top_builddir)/fawnkv			\
	-I$(top_builddir)/fawnds		\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp \
	-I$(top_builddir)/test/siltds/gen-cpp

testByYCSBPUT_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/fawnkv/libfawnkv.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(top_builddir)/siltds/libsiltcluster.la 	\
	-lthrift 			\
	$(THRIFT_LIBS)  

benchCuckoo_SOURCES = benchCuckoo.cc
benchCuckoo_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

benchCuckoo_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)

benchSemiRandomWrite_SOURCES = benchSemiRandomWrite.cc
benchSemiRandomWrite_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

benchSemiRandomWrite_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)

benchStores_SOURCES = benchStores.cc
benchStores_CPPFLAGS = 			\
	-I$(top_srcdir)/utils 			\
	-I$(top_srcdir)/fawnds			\
	-I$(top_builddir)/siltds		\
	-I$(top_builddir)/siltds/gen-cpp

benchStores_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
	$(THRIFT_LIBS)


preprocessTrace_SOURCES= preprocessTrace.cc

preprocessTrace_CPPFLAGS =

preprocessTrace_LDADD =


#CLEANFILES = $(nodist_libfawnkv_la_SOURCES) $(SKELETON_CODE)

