#
# kexec_elf32_x86 (linux booting linux)
#
KEXEC_C_SRCS:= kexec/kexec.c kexec/kexec-syscall.c 
KEXEC_S_SRCS:= 
KEXEC_C_SRCS+= kexec/kexec-x86.c kexec/kexec-elf32-x86.c kexec/kexec-bzImage.c
KEXEC_C_SRCS+= kexec/ifdown.c
KEXEC_S_SRCS+= kexec/x86-setup-32.S kexec/x86-setup-16.S kexec/x86-setup-16-debug.S
KEXEC_C_OBJS:= $(patsubst %.c, $(OBJDIR)/%.o, $(KEXEC_C_SRCS))
KEXEC_C_DEPS:= $(patsubst %.c, $(OBJDIR)/%.d, $(KEXEC_C_SRCS))
KEXEC_S_OBJS:= $(patsubst %.S, $(OBJDIR)/%.o, $(KEXEC_S_SRCS))
KEXEC_S_DEPS:= $(patsubst %.S, $(OBJDIR)/%.d, $(KEXEC_S_SRCS))
KEXEC_SRCS:= $(KEXEC_C_SRCS) $(KEXEC_S_SRCS)
KEXEC_OBJS:= $(KEXEC_C_OBJS) $(KEXEC_S_OBJS)
KEXEC_DEPS:= $(KEXEC_C_DEPS) $(KEXEC_S_DEPS)
KEXEC:= $(SBINDIR)/kexec

include $(KEXEC_DEPS)

$(KEXEC_C_DEPS): $(OBJDIR)/%.d: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -M $< | sed -e 's|$(patsubst %.d,%.o,$(@F))|$(patsubst %.d,%.o,$(@))|' > $@

$(KEXEC_S_DEPS): $(OBJDIR)/%.d: %.S
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -M $< | sed -e 's|$(patsubst %.d,%.o,$(@F))|$(patsubst %.d,%.o,$(@))|' > $@

$(KEXEC_C_OBJS): $(OBJDIR)/%.o: %.c $(OBJDIR)/%.d
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(KEXEC_S_OBJS): $(OBJDIR)/%.o: %.S $(OBJDIR)/%.d
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(KEXEC): $(KEXEC_OBJS) $(UTIL_LIB)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -static -o $@ $(KEXEC_OBJS) $(UTIL_LIB)

echo::
	@echo "KEXEC_C_SRCS $(KEXEC_C_SRCS)"
	@echo "KEXEC_C_DEPS $(KEXEC_C_DEPS)"
	@echo "KEXEC_C_OBJS $(KEXEC_C_OBJS)"
	@echo "KEXEC_S_SRCS $(KEXEC_S_SRCS)"
	@echo "KEXEC_S_DEPS $(KEXEC_S_DEPS)"
	@echo "KEXEC_S_OBJS $(KEXEC_S_OBJS)"
	@echo "KEXEC_SRCS $(KEXEC_SRCS)"
	@echo "KEXEC_DEPS $(KEXEC_DEPS)"
	@echo "KEXEC_OBJS $(KEXEC_OBJS)"

