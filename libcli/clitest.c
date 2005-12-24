#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include "libcli.h"

#define MODE_CONFIG_INT		10

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

int cmd_test(struct cli_def *cli, char *command, char *argv[], int argc)
{
    int i;
    cli_print(cli, "called %s with \"%s\"", __FUNCTION__, command);
    cli_print(cli, "%d arguments:", argc);
    for (i = 0; i < argc; i++)
    {
	cli_print(cli, "	%s", argv[i]);
    }
    return CLI_OK;
}

int cmd_set(struct cli_def *cli, char *command, char *argv[], int argc)
{
    if (argc < 2)
    {
	cli_print(cli, "Specify a variable to set");
	return CLI_OK;
    }
    cli_print(cli, "Setting \"%s\" to \"%s\"", argv[0], argv[1]);
    return CLI_OK;
}

int cmd_config_int(struct cli_def *cli, char *command, char *argv[], int argc)
{
	if (argc < 1)
	{
		cli_print(cli, "Specify an interface to configure");
		return CLI_OK;
	}
	if (strcmp(argv[0], "?") == 0)
	{
		cli_print(cli, "  test0/0");
	}
	else if (strcasecmp(argv[0], "test0/0") == 0)
	{
		cli_set_configmode(cli, MODE_CONFIG_INT, "test");
	}
	else
		cli_print(cli, "Unknown interface %s", argv[0]);
	return CLI_OK;
}

int cmd_config_int_exit(struct cli_def *cli, char *command, char *argv[], int argc)
{
	cli_set_configmode(cli, MODE_CONFIG, NULL);
	return CLI_OK;
}

int check_auth(char *username, char *password)
{
    if (!strcasecmp(username, "fred") && !strcasecmp(password, "nerk"))
	return 1;
    return 0;
}

int check_enable(char *password)
{
    if (!strcasecmp(password, "topsecret"))
        return 1;
    return 0;
}

void pc(struct cli_def *cli, char *string)
{
	printf("%s\n", string);
}

int main(int argc, char *argv[])
{
    struct cli_command *c;
    struct cli_def *cli;

    cli = cli_init();
    cli_set_banner(cli, "libcli test environment");
    cli_set_hostname(cli, "router");
    cli_set_newline(cli, "\n");
    cli_register_command(cli, NULL, "test", cmd_test, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, NULL, "sex",  NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, NULL, "simple",  NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, NULL, "simon",  NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, NULL, "set", cmd_set,  PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    c = cli_register_command(cli, NULL, "show", NULL,  PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, c, "counters", cmd_test, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show the counters that the system uses");
    cli_register_command(cli, c, "junk", cmd_test, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);


    cli_register_command(cli, NULL, "interface", cmd_config_int, PRIVILEGE_PRIVILEGED, MODE_CONFIG, "Configure an interface");
    cli_register_command(cli, NULL, "exit", cmd_config_int_exit, PRIVILEGE_PRIVILEGED, MODE_CONFIG_INT, "Exit from interface configuration");
    cli_register_command(cli, NULL, "address", cmd_test, PRIVILEGE_PRIVILEGED, MODE_CONFIG_INT, "Set IP address");

    //cli_set_auth_callback(cli, check_auth);
    //cli_set_enable_callback(cli, check_enable);
    // Test reading from a file
    {
	    FILE *fh;

	    if ((fh = fopen("clitest.txt", "r")))
	    {
		    // This sets a callback which just displays the cli_print() text to stdout
		    cli_print_callback(cli, pc);
		    cli_file(cli, fh, PRIVILEGE_UNPRIVILEGED, MODE_EXEC);
		    cli_print_callback(cli, NULL);
		    fclose(fh);
	    }
    }

    set_canonical(0);
    cli_loop(cli, fileno(stdin), fileno(stdout));
    set_canonical(1);
        
    cli_done(cli);
    return 0;
}

