#
# Utility function library
#
UTIL_LIB_SRCS:=util_lib/compute_ip_checksum.c
UTIL_LIB_OBJS:=$(patsubst %.c, $(OBJDIR)/%.o, $(UTIL_LIB_SRCS))
UTIL_LIB_DEPS:=$(patsubst %.c, $(OBJDIR)/%.d, $(UTIL_LIB_SRCS))
UTIL_LIB:=$(OBJDIR)/libutil.a

include $(UTIL_LIB_DEPS)

$(UTIL_LIB_DEPS): $(OBJDIR)/%.d: %.c 
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -M $< | sed -e 's|$(patsubst %.d,%.o,$(@F))|$(patsubst %.d,%.o,$(@))|' > $@

$(UTIL_LIB_OBJS): $(OBJDIR)/%.o: %.c $(OBJDIR)/%.d
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(UTIL_LIB): $(UTIL_LIB_OBJS)
	mkdir -p $(@D)
	$(AR) rs $(UTIL_LIB) $(UTIL_LIB_OBJS)

ifdef X86_CC
X86_UTIL_LIB_OBJS:=$(patsubst %.c, $(X86_OBJDIR)/%.o, $(UTIL_LIB_SRCS))
X86_UTIL_LIB_DEPS:=$(patsubst %.c, $(X86_OBJDIR)/%.d, $(UTIL_LIB_SRCS))
X86_UTIL_LIB:=$(X86_OBJDIR)/libutil.a

include $(X86_UTIL_LIB_DEPS)

$(X86_UTIL_LIB_DEPS): $(X86_OBJDIR)/%.d: %.c 
	mkdir -p $(@D)
	$(X86_CC) $(X86_CFLAGS) -M $< | sed -e 's|$(patsubst %.d,%.o,$(@F))|$(patsubst %.d,%.o,$(@))|' > $@

$(X86_UTIL_LIB_OBJS): $(X86_OBJDIR)/%.o: %.c $(X86_OBJDIR)/%.d
	mkdir -p $(@D)
	$(X86_CC) $(X86_CFLAGS) -c -o $@ $<

$(X86_UTIL_LIB): $(X86_UTIL_LIB_OBJS)
	mkdir -p $(@D)
	$(X86_AR) rs $(X86_UTIL_LIB) $(X86_UTIL_LIB_OBJS)

endif

ifdef ALPHA_CC
ALPHA_UTIL_LIB_OBJS:=$(patsubst %.c, $(ALPHA_OBJDIR)/%.o, $(UTIL_LIB_SRCS))
ALPHA_UTIL_LIB_DEPS:=$(patsubst %.c, $(ALPHA_OBJDIR)/%.d, $(UTIL_LIB_SRCS))
ALPHA_UTIL_LIB:=$(ALPHA_OBJDIR)/libutil.a

include $(ALPHA_UTIL_LIB_DEPS)

$(ALPHA_UTIL_LIB_DEPS): $(ALPHA_OBJDIR)/%.d: %.c 
	mkdir -p $(@D)
	$(ALPHA_CC) $(ALPHA_CFLAGS) -M $< | sed -e 's|$(patsubst %.d,%.o,$(@F))|$(patsubst %.d,%.o,$(@))|' > $@

$(ALPHA_UTIL_LIB_OBJS): $(ALPHA_OBJDIR)/%.o: %.c $(ALPHA_OBJDIR)/%.d
	mkdir -p $(@D)
	$(ALPHA_CC) $(ALPHA_CFLAGS) -c -o $@ $<

$(ALPHA_UTIL_LIB): $(ALPHA_UTIL_LIB_OBJS)
	mkdir -p $(@D)
	$(ALPHA_AR) rs $(ALPHA_UTIL_LIB) $(ALPHA_UTIL_LIB_OBJS)

endif
