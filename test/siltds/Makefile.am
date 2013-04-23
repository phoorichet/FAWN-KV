#hashdb_test code
noinst_PROGRAMS = testSilt testFawnDS testIterator testTrie testCuckoo testCombi testByYCSBWorkload benchCuckoo benchSemiRandomWrite benchStores preprocessTrace
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
	-I$(top_builddir)/siltds/gen-cpp 

testByYCSBWorkload_LDADD = 				\
	$(top_builddir)/siltds/libfawnds.la 	\
	$(top_builddir)/fawnkv/libfawnkv.la 	\
	$(top_builddir)/utils/libfawnkvutils.la \
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

