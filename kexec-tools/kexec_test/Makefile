#
# kexec_test Debugging payload to be certain the infrastructure works
#
RELOC:=0x10000
KEXEC_TEST_S_SRCS:= kexec_test/kexec_test16.S kexec_test/kexec_test.S
KEXEC_TEST_S_TEMPS:=$(patsubst %.S, $(OBJDIR)/%.s, $(KEXEC_TEST_S_SRCS))
KEXEC_TEST_S_OBJS:=$(patsubst $(OBJDIR)/%.s, $(OBJDIR)/%.o, $(KEXEC_TEST_S_TEMPS))
KEXEC_TEST_S_DEPS:=$(patsubst %.S, $(OBJDIR)/%.d, $(KEXEC_TEST_S_SRCS))
KEXEC_TEST_SRCS:= $(KEXEC_TEST_S_SRCS)
KEXEC_TEST_OBJS:= $(KEXEC_TEST_S_OBJS)
KEXEC_TEST_DEPS:= $(KEXEC_TEST_S_DEPS)
KEXEC_TEST:=$(BINDIR)/kexec_test

include $(KEXEC_TEST_DEPS)

$(KEXEC_TEST_S_DEPS): $(OBJDIR)/%.d: %.S
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -M $< | sed -e 's|$(patsubst %.d,%.o,$(@F))|$(patsubst %.d,%.o,$(@))|' > $@

$(KEXEC_TEST_S_TEMPS): $(OBJDIR)/%.s: %.S 
	mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) -DRELOC=$(RELOC) $< > $@

$(KEXEC_TEST_S_OBJS): $(OBJDIR)/%.o: $(OBJDIR)/%.s 
	mkdir -p $(@D)
	$(AS) -o $@ $<

$(KEXEC_TEST): $(KEXEC_TEST_OBJS) 
	mkdir -p $(@D)
	$(LD) -e _start -Ttext $(RELOC) $(KEXEC_TEST_OBJS) -o $@
