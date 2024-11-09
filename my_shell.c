#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

/* Read a line of characters from stdin. */
int getcmd(char *buf, int nbuf)
{

  // Prompt for user to input command
  // clearing buf
  for (int i = 0; i < 100; i++)
  {
    buf[i] = 0;
  }
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
    for (int j = 0; j < 100; j++)
    {
      arguments[i][j] = '\0';
    }
  }

  int numargs = 0;
  /* Word start/end */
  int ws = 1;
  int we = 1; //changed from 0
  int flagPos = 0;

  int redirection_left = 0;
  int redirection_right = 0;
  // char *file_name_l = 0;
  // char *file_name_r = 0;

  int p[2];

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
          if (!flagPos)
          {
            flagPos = i;
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
        else if (buf[i] == '\n')
        {
          buf[i] = '\0';
        }

        if (ws != -1)
        {
          we = i;
          if (i == 0)
          {
            numargs = -1;
          }
          else
          {
            arguments[numargs][i - ws] = '\0';
          }
          numargs += 1;
          ws = -1;
        }
      }
      //parses single character into respective osition in argument array
      else
      {
        if (we != -1)
        {
          ws = i;
          we = -1;
        }
        arguments[numargs][i - ws] = buf[i];
      }
    }
    else
    {
      /* Redirection command. Capture the file names. */
    }
  }
  arguments[numargs] = 0;

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
  //debugging

  printf("Command: %s\n", arguments[0]);
  for (int i = 0; i < numargs; i++)
  {
    printf("Argument %d: %s\n", i, arguments[i]);
  }

  /*
    If this command is a CD command, write the arguments to the pcp pipe
    and exit with '2' to tell the parent process about this.
  */
  if (strcmp(arguments[0], "cd") == 0)
  {
    close(pcp[0]);
    write(pcp[1], buf, nbuf);
    close(pcp[1]);
    exit(2);
  }
  else
  {
    /*
      Pipe command: fork twice. Execute the left hand side directly.
      Call run_command recursion for the right side of the pipe.
    */
    if (pipe_cmd)
    {
      pipe(p);

      int pID[2];
      pID[0] = fork();
      if (pID[0] == 0)
      {
        close(1);
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        exec(arguments[0], arguments);
        exit(0);
      }
      else
      {

        pID[1] = fork();
        if (pID[1] == 0)
        {
          close(0);
          dup(p[0]);
          close(p[1]);
          close(p[0]);
          run_command(buf + (flagPos + 1), nbuf - flagPos - 1, pcp);
          exit(0);
        }

        close(p[0]);
        close(p[1]);
        wait(0);
      }
    }
    else
    {
      // allows user to exit the shell, otherwise it is indefinitely looping
      if (strcmp(arguments[0], "exit") == 0)
      {
        // its 'exi' just so it fits the pcpArgs0] properly in main
        close(pcp[0]);
        write(pcp[1], "exi", 3);
        close(pcp[1]);
        exit(0);
      }
      exec(arguments[0], arguments);
      printf("Exec failed\n");
    }
  }
  exit(0);
}

int main(void)
{
  int pcp[2];
  pipe(pcp);
  while (1)
  {
    static char buf[100];

    /* Read and run input commands. */
    // indefinite loop, inside 'run_command' there is a specific exit option
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
      int child_status = fork();
      printf("%d\n", child_status);
      if (child_status == 0)
      {
        run_command(buf, 100, pcp);
      }
      else
      {

        wait(0);

        char *pcpArgs[2];
        for (int i = 0; i < 2; i++)
        {
          pcpArgs[i] = malloc(100 * sizeof(char));
          for (int j = 0; j < 100; j++)
          {
            pcpArgs[i][j] = '\0';
          }
        }
        int len = 0;
        len = read(pcp[0], pcpArgs[0], 3);
        if (!len)
        {
        close(pcp[0]);
        write(pcp[1], "pas", 3);
        close(pcp[1]);
        exit(0);
        }
        else
        {
          if (pcpArgs[0][0] == 'c' && pcpArgs[0][1] == 'd')
          {
            //int len = 0;
            read(pcp[0], pcpArgs[1], sizeof(pcpArgs[1]));
            close(pcp[0]);
            close(pcp[1]);
            if (chdir(pcpArgs[1]) != 0)
            {
              printf("directory change failed\n");
            }
          }
          else if (pcpArgs[0][0] == 'e' && pcpArgs[0][1] == 'x' && pcpArgs[0][2] == 'i')
          {
            exit(0);
          }
          else if (pcpArgs[0][0] == 'p' && pcpArgs[0][1] == 'a' && pcpArgs[0][2] == 's')
          {
            // pipe concents 'pas' just mean that the pipe was empty beforehand, so is 'passing' through to the next command input
          }
          else
          {
            printf("invalid use of pcp\n");
          }
          close(pcp[0]);
          close(pcp[1]);
        }
      }
    }
  }
}
//error with commands, blank spac after? not loping properly? exit? idk