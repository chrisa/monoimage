#define C99_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <setjmp.h>
#include <libcli.h>
#include <confuse.h>

#include "monoboot.h"

extern cfg_t *cfg;

/* $Id$ */

/* get keypress with timeout */

static jmp_buf env_alrm;
static void sig_alrm(int signo) {
    longjmp(env_alrm, 1);
}
  
int get_keypress (int delay) {
    int c = 0;
    
    if (!delay)
	return 1;

    set_canonical(0);
    if (setvbuf(stdin, NULL, _IONBF, 0) < 0) {
	fprintf(stderr, "unable to setvbuf: %s\n", strerror(errno));
    }
    
    printf("MONOBOOT booting in %ds\npress a key for a shell", delay);
    while (delay--) {
	printf(".");
        if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
	    fprintf(stderr, "signal: %s\n", strerror(errno));
	    set_canonical(1);
	    return(-1);
	}
	if (setjmp(env_alrm) == 0) {
	    alarm(1);
	    c = getchar();
	}
	if (c) {
	    alarm(0);
	    printf("\ngot keypress\n");
	    set_canonical(1);
	    return(0);
	}
    }

    printf("\n");
    set_canonical(1);
    return(1);
}

/* set/unset ICANON and ECHO flags */
void set_canonical(int flag) {
    struct termios term;
    
    if (tcgetattr(STDIN_FILENO, &term) < 0) {
	fprintf(stderr, "unable to tcgetattr: %s\n", strerror(errno));
    }

    if (flag) {
	term.c_lflag |= ICANON;
	term.c_lflag |= ECHO;
    } else {
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
	fprintf(stderr, "unable to tcsetattr: %s\n", strerror(errno));
    }
}

int cli_cmd_boot(struct cli_def *cli, char *command, char *argv[], int argc)
{
        return cmd_boot(cli, cfg, argv);
}

int cli_cmd_show_running_config(struct cli_def *cli, char *command, char *argv[], int argc)
{
        return cmd_show_run(cli, cfg, argv);
}

int cli_cmd_show_startup_config(struct cli_def *cli, char *command, char *argv[], int argc)
{
        return cmd_show_start(cli, cfg, argv);
}

int cli_cmd_show_version(struct cli_def *cli, char *command, char *argv[], int argc)
{
        return cmd_show_ver(cli, cfg, argv);
}

int cli_cmd_show_disk(struct cli_def *cli, char *command, char *argv[], int argc)
{
        return cmd_show_disk(cli, cfg, argv);
}

int cli_cmd_write(struct cli_def *cli, char *command, char *argv[], int argc)
{
        return cmd_write(cli, cfg, argv);
}

int cli_cmd_shell(struct cli_def *cli, char *command, char *argv[], int argc)
{
        return cmd_shell(cli, cfg, argv);
}

int cli_cmd_config_image(struct cli_def *cli, char *command, char *argv[], int argc)
{
	if (argc < 1)
	{
		cli_print(cli, "Specify an image to configure");
		return CLI_OK;
	}

        cli_set_configmode(cli, MODE_CONFIG_IMAGE, argv[1]);
	return CLI_OK;
}

int cli_cmd_config_net(struct cli_def *cli, char *command, char *argv[], int argc)
{
        cli_set_configmode(cli, MODE_CONFIG_NET, NULL);
	return CLI_OK;
}

int cli_cmd_config_mode_exit(struct cli_def *cli, char *command, char *argv[], int argc)
{
	cli_set_configmode(cli, MODE_CONFIG, NULL);
	return CLI_OK;
}

void mb_interact(cfg_t *cfg)
{
        struct cli_command *c;
        struct cli_def *cli;

        cli = cli_init();
        cli_set_banner(cli, "monoboot shell");
        cli_set_hostname(cli, "mb");
        cli_set_newline(cli, "\n");
        
        cli_register_command(cli, NULL, "boot", cli_cmd_boot, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

        c = cli_register_command(cli, NULL, "show", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
        cli_register_command(cli, c, "running-config", cli_cmd_show_running_config, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
        cli_register_command(cli, c, "startup-config", cli_cmd_show_startup_config, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
        cli_register_command(cli, c, "version", cli_cmd_show_version, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
        cli_register_command(cli, c, "disk", cli_cmd_show_disk, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
        
        cli_register_command(cli, NULL, "write", cli_cmd_write, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
        cli_register_command(cli, NULL, "shell", cli_cmd_shell, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

        c = cli_register_command(cli, NULL, "copy", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
        
        cli_register_command(cli, NULL, "image", cli_cmd_config_image, PRIVILEGE_PRIVILEGED, MODE_CONFIG, "Create an image tag");
        cli_register_command(cli, NULL, "exit", cli_cmd_config_mode_exit, PRIVILEGE_PRIVILEGED, MODE_CONFIG_IMAGE, "Exit from image configuration");
        cli_register_command(cli, NULL, "network", cli_cmd_config_net, PRIVILEGE_PRIVILEGED, MODE_CONFIG, "Configure the network");
        cli_register_command(cli, NULL, "exit", cli_cmd_config_mode_exit, PRIVILEGE_PRIVILEGED, MODE_CONFIG_NET, "Exit from network configuration");
        
        set_canonical(0);
        cli_loop(cli, STDIN_FILENO, STDOUT_FILENO);
        set_canonical(1);
        
        cli_done(cli);
}
