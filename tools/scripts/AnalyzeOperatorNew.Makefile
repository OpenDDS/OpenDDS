DDS_LIBS = Corba Dcps FACE Rtps Rtps_Udp
DDS_REPORTS = $(foreach lib,$(DDS_LIBS),$(lib).report)
DDS_DUMPS = $(foreach lib,$(DDS_LIBS),$(lib).dds.dump)

ACE_LIBS = ACE
ACE_REPORTS = $(foreach lib,$(ACE_LIBS),$(lib).report)
ACE_DUMPS = $(foreach lib,$(ACE_LIBS),$(lib).ace.dump)

REPORTS = $(DDS_REPORTS) $(ACE_REPORTS)
DUMPS = $(DDS_DUMPS) $(ACE_DUMPS)

all: report
	cat report

report: $(REPORTS)
	wc -l $^ > $@

%.dds.dump: $(DDS_ROOT)/lib/libOpenDDS_%.so
	objdump -C -l -d -S $< | c++filt > $@

%.ace.dump: $(ACE_ROOT)/lib/lib%.so
	objdump -C -l -d -S $< | c++filt > $@

%.report: %.dds.dump
	cat $< | awk -f $(DDS_ROOT)/tools/scripts/find_operator_new.awk | sort | uniq -c > $@

%.report: %.ace.dump
	cat $< | awk -f $(DDS_ROOT)/tools/scripts/find_operator_new.awk | sort | uniq -c > $@

.SECONDARY: $(DUMPS) $(REPORTS)

clean:
	-rm -f report $(REPORTS) $(DUMPS)
