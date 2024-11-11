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
  read(0, buf, nbuf);
  return nbuf;
}

/*
  A recursive function which parses the command
  at *buf and executes it.
*/
__attribute__((noreturn)) void run_command(char *buf, int nbuf, int *pcp)
{

  //memory allocating my 'strings'
  char *arguments[10];
  char *filename;
  for (int i = 0; i < 10; i++)
  {
    arguments[i] = malloc(100 * sizeof(char));
    for (int j = 0; j < 100; j++)
    {
      arguments[i][j] = '\0';
    }
  }

  filename = malloc(100 * sizeof(char));
  for (int j = 0; j < 100; j++)
  {
    filename[j] = '\0';
  }

  int numargs = 0;
  /* Word start/end */
  int ws = 1;
  int we = 1;      //changed from 0 for functional purposes
  int flagPos = 0; // flagging certain characters to break from loop
  int redirection_left = 0;
  int fileread = 0; // detect first charatcer being read into filename
  int redirection_right = 0;
  int f[2]; // for file usage
  int p[2];

  int pipe_cmd = 0;

  int sequence_cmd = 0;

  int i = 0;
  /* Parse the command character by character.*/
  for (i = 0; i < nbuf; i++)
  {

    if (buf[i] == '>')
    {
      redirection_right = 1;
      if (numargs == 0)
      {
        printf("invalid input");
        exit(1);
      }
    }
    else if (buf[i] == '<')
    {
      redirection_left = 1;
      if (numargs == 0)
      {
        printf("invalid input");
        exit(1);
      }
    }
    /* Parse the current character and set-up various flags:
       sequence_cmd, redirection, pipe_cmd and similar. */
    if (!(redirection_left || redirection_right))
    {
      if (buf[i] == ' ' || buf[i] == '|' || buf[i] == ';' || buf[i] == '\n' || buf[i] == '\0' || buf[i] == '<' || buf[i] == '>')
      {
        if (buf[i] == '|')
        {
          pipe_cmd = 1;
          if (numargs == 0)
          {
            printf("invalid input");
            exit(1);
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
            printf("invalid input");
            exit(1);
          }
          if (!flagPos)
          {
            flagPos = i;
          }
        }
        else if (buf[i] == '\n')
        {
          buf[i] = '\0'; // getting rid of newline so arguments can be read properly
        }

        if (ws != -1) //checking for end of new word
        {
          we = i;
          if (i == 0)
          {
            numargs = -1;
          }
          else
          {
            arguments[numargs][i - ws] = '\0'; // ending string with null terminator
          }
          numargs += 1;
          ws = -1;
        }
        if (flagPos)
        {
          break; // other parts of buffer will be held on to and executed later
        }
      }
      else
      {
        if (we != -1) //checking for start of new word
        {
          ws = i;
          we = -1;
        }
        arguments[numargs][i - ws] = buf[i]; //parses single character into respective position in argument array
      }
    }
    else
    {
      if (buf[i] == ' ' || buf[i] == '|' || buf[i] == ';' || buf[i] == '\n' || buf[i] == '\0' || buf[i] == '<' || buf[i] == '>')
      {

        if (buf[i] == '\n')
        {
          buf[i] = '\0';
        }

        if (ws != -1)
        {
          we = i;
          if (i == 0)
          {
            printf("invalid input");
            exit(1);
          }
          else if (fileread)
          {
            filename[i - ws] = '\0'; //filename complete, break from loop
            break;
          }
          else
          {
            arguments[numargs][i - ws] = '\0';
            numargs += 1;
            ws = -1;
          }
        }
      }
      else
      {
        if (we != -1)
        {
          ws = i;
          we = -1;
          fileread = 1;
        }
        filename[i - ws] = buf[i]; // adding char by char to filename
      }
    }
  }

  arguments[numargs] = 0; //just inacse, null terminaning the next array after the last argumnt
  //   printf("Filename: %s\n", filename);
  // for (int i = 0; i < numargs; i++)          <----This was my debugging code
  // {
  //   printf("Argument %d: %s\n", i, arguments[i]);
  // }

  if (sequence_cmd) // fork off to execute one command, then carry on reading rest of buffer for other commands
  {
    sequence_cmd = 0;
    if (fork() == 0)
    {
      exec(arguments[0], arguments);
      exit(0);
    }
    else
    {
      wait(0);
      run_command(buf + (flagPos + 1), nbuf - flagPos - 1, pcp); // cuts buffer from where flagged onward such to split the commands at the ; character
    }
  }

  if (redirection_left)
  {
    close(0);
    f[0] = open(filename, O_RDONLY);
    if (f[0] < 0)
    {
      printf("error reading file");
      exit(1);
    }

    dup(f[0]); //directing file contents as next input
    close(f[0]);
  }
  if (redirection_right)
  {
    close(1);
    f[1] = open(filename, O_CREATE | O_WRONLY);
    if (f[1] < 0)
    {
      printf("error opening/creating file");
      exit(1);
    }

    dup(f[1]); // setting up such that next exec() contents are filtered into file
    close(f[1]);
  }

  if (strcmp(arguments[0], "cd") == 0)
  {
    close(pcp[0]);
    write(pcp[1], buf, nbuf); //cd command called in parent through using parentChildPipe
    close(pcp[1]);
    exit(2);
  }
  else
  {

    if (pipe_cmd)
    {
      pipe(p); //left fork

      int pID[2];
      pID[0] = fork();
      if (pID[0] == 0)
      {
        close(1);
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        exec(arguments[0], arguments); //directing command executing output into the left side of pipe (read)
        exit(0);
      }
      else
      {

        pID[1] = fork();
        if (pID[1] == 0)
        {
          close(0);
          dup(p[0]);
          close(p[1]); //accessing right side of pipe (write) such that it is used as next standard input in next command
          close(p[0]);
          run_command(buf + (flagPos + 1), nbuf - flagPos - 1, pcp); //recursion
          exit(0);
        }

        close(p[0]);
        close(p[1]);
        wait(0);
        wait(0); // double wait becasue forks twice, dont want final command to output after the next user input is prompted
      }
    }
    else
    {
      // allows user to exit the shell, otherwise it is indefinitely looping
      if (strcmp(arguments[0], "exit") == 0)
      {
        // its 'exi' just so it fits the pcpArgs[0] properly in main
        close(pcp[0]);
        write(pcp[1], "exi", 3);
        close(pcp[1]);
        exit(0);
      }
      close(pcp[0]);
      write(pcp[1], "pas", 3); // passing 'pas' through pipe as a standard instruction to 'pass' along when the pcp is checking in the parent instead of calling an error
      close(pcp[1]);
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
  while (1) // indefinite loop, escape with exit command
  {
    static char buf[100];
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
      int child_status = fork();
      // printf("%d\n", child_status); --> More debugging printf
      if (child_status == 0)
      {
        run_command(buf, 100, pcp);
      }
      else
      {
        wait(0);
        char *pcpArgs[2]; //for recieving cd and exit commands
        for (int i = 0; i < 2; i++)
        {
          pcpArgs[i] = malloc(100 * sizeof(char));
          for (int j = 0; j < 100; j++)
          {
            pcpArgs[i][j] = '\0';
          }
        }
        int len = 0; // strong first three characters of pcp from child
        len = read(pcp[0], pcpArgs[0], 3);
        close(pcp[1]);
        if (!len)
        {
          printf("pcp read error");
        }
        else
        {
          if (pcpArgs[0][0] == 'c' && pcpArgs[0][1] == 'd')
          {
            read(pcp[0], pcpArgs[1], sizeof(pcpArgs[1]) - 3); // now reading directory for change
            close(pcp[0]);
            if (chdir(pcpArgs[1]) != 0)
            {
              printf("directory change failed\n");
            }
          }
          else if (pcpArgs[0][0] == 'e' && pcpArgs[0][1] == 'x' && pcpArgs[0][2] == 'i')
          {
            exit(0); //simple enough
          }
          else if (pcpArgs[0][0] == 'p' && pcpArgs[0][1] == 'a' && pcpArgs[0][2] == 's')
          {
            //as said previous, lets parent 'pass' through as this means the pcp has not been used for cd or exit command
          }
          else
          {
            printf("invalid use of pcp\n"); // just incase
          }
        }
        close(pcp[0]);
        pipe(pcp); // refresh pipe to prevent overflow issue when entering multiple commands
      }
    }
  }
}