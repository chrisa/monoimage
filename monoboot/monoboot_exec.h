/* needs these binaries */

#define KEXEC_BINARY "/usr/sbin/kexec"
#define TFTP_BINARY  "/usr/bin/tftp"
#define SCP_BINARY   "/usr/bin/scp"
#define IP_BINARY    "/bin/ip"

int do_exec (const char*, const char*, ...);

int do_tftp (cfg_t *, char *, char *);
int do_netconf (cfg_t *);
