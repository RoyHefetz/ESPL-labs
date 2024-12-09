#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <string.h>

void handler(int sig)
{
	printf("\nRecieved Signal : %s\n", strsignal(sig));
	if (sig == SIGCONT) {
        signal(SIGTSTP, handler); // Re-instate custom handler for SIGTSTP
    } else if (sig== SIGTSTP) {
        signal(SIGCONT, handler); // Re-instate custom handler for SIGCONT
    }
	signal(sig, SIG_DFL);
	raise(sig);
}

int main(int argc, char **argv)
{

	printf("Starting the program\n");
	signal(SIGINT, handler);
	signal(SIGTSTP, handler);
	signal(SIGCONT, handler);

	while (1)
	{
		sleep(1);
	}

	return 0;
}