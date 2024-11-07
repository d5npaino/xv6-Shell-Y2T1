#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

/* Read a line of characters from stdin. */
int getcmd(char *buf, int nbuf)
{

  // Prompt for user to input command
  printf(">>> ");
  // fgets(buf, nbuf, stdin);
  read(0, buf, nbuf);
  return nbuf;
}

/*
  A recursive function which parses the command
  at *buf and executes it.
*/
__attribute__((noreturn)) void run_command(char *buf, int nbuf, int *pcp)
{

  /* Useful data structures and flags. */
  char *arguments[10];
  for (int i = 0; i < 10; i++)
  {
    arguments[i] = malloc(100 * sizeof(char));
    if (!arguments[i])
    {
      // return error message failed to allocate memory
    }
  }

  int numargs = 0;
  /* Word start/end */
  int ws = 1;
  int we = 1; //changed from 0

  int redirection_left = 0;
  int redirection_right = 0;
  // char *file_name_l = 0;
  // char *file_name_r = 0;

  // int p[2];
  int pipe_cmd = 0;

  int sequence_cmd = 0;

  int i = 0;
  /* Parse the command character by character.*/
  for (i = 0; i < nbuf; i++)
  {

    /* Parse the current character and set-up various flags:
       sequence_cmd, redirection, pipe_cmd and similar. */

    if (buf[i] == '>')
    {
      redirection_right = 1;
      if (numargs == 0)
      {
        /* throw error */
      }
    }
    else if (buf[i] == '<')
    {
      redirection_left = 1;
      if (numargs == 0)
      {
        /* throw error */
      }
    }

    if (!(redirection_left || redirection_right))
    {
      /* No redirection, continue parsing command. */

      if (buf[i] == ' ' || buf[i] == '|' || buf[i] == ';' || buf[i] == '\n' || buf[i] == '\0')
      {
        if (buf[i] == '|')
        {
          pipe_cmd = 1;
          if (numargs == 0)
          {
            /* throw error */
          }
        }
        else if (buf[i] == ';')
        {
          sequence_cmd = 1;
          if (numargs == 0)
          {
            /* throw error */
          }
        }

        if (ws != 0)
        {
          we = ws;
          ws = 0;
          numargs += 1;
          if (i == 0)
          {
            numargs -= 1;
          }
        }
      }
      //parses single character into respective osition in argument array
      else
      {
        if (we != 0)
        {
          ws = i;
          we = 0;
        }
        arguments[numargs][i - ws] = buf[i];
      }
    }
    else
    {
      /* Redirection command. Capture the file names. */
    }
  }

  /*
    Sequence command. Continue this command in a new process.
    Wait for it to complete and execute the command following ';'.
  */
  if (sequence_cmd)
  {
    sequence_cmd = 0;
    if (fork() != 0)
    {
      wait(0);
      // ##### Place your code here.
    }
  }

  /*
    If this is a redirection command,
    tie the specified files to std in/out.
  */
  if (redirection_left)
  {
    // ##### Place your code here.
  }
  if (redirection_right)
  {
    // ##### Place your code here.
  }

  /* Parsing done. Execute the command. */

  /*
    If this command is a CD command, write the arguments to the pcp pipe
    and exit with '2' to tell the parent process about this.
  */
  if (strcmp(arguments[0], "cd") == 0)
  {
    // ##### Place your code here.
  }
  else
  {
    /*
      Pipe command: fork twice. Execute the left hand side directly.
      Call run_command recursion for the right side of the pipe.
    */
    if (pipe_cmd)
    {
      // ##### Place your code here.
    }
    else
    {
      // allows user to exit the shell, otherwise it is indefinitely looping
      if (strcmp(arguments[0], "exit") == 0)
      {
        exit(0);
      }
       printf("Command: %s\n", arguments[0]);
      exec(arguments[0], arguments);
    }
  }
  exit(0);
}

int main(void)
{

  static char buf[100];

  int pcp[2];
  pipe(pcp);

  /* Read and run input commands. */
  // indefinite loop, inside 'run_command' there is a specific exit option
  while (1)
  {
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
      if (fork() == 0)
        run_command(buf, 100, pcp);

      /*
      Check if run_command found this is
      a CD command and run it if required.
    */
     // int child_status;
      // ##### Place your code here
    }
  }
  exit(0);
}
