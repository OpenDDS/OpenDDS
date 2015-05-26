LIBS = Corba Dcps FACE Rtps Rtps_Udp
REPORTS = $(foreach lib,$(LIBS),$(lib).report)
DUMPS = $(foreach lib,$(LIBS),$(lib).dump)

all: report
	cat report

report: $(REPORTS)
	wc -l $^ > $@

%.dump: $(DDS_ROOT)/lib/libOpenDDS_%.so
	objdump -C -l -d -S $< | c++filt > $@

%.report: %.dump
	cat $< | awk -f $(DDS_ROOT)/tools/scripts/find_operator_new.awk | sort | uniq -c > $@

.SECONDARY: $(DUMPS) $(REPORTS)

clean:
	-rm -f report $(REPORTS) $(DUMPS)
