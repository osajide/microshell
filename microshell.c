#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int	ft_strlen(char *s)
{
	int	i;

	i = 0;
	while (s[i])
		i++;
	return (i);
}

int	give_semi_colon_pos(char **av, int i, int *pipe_check)
{
	*pipe_check = 0;
	while (av[i])
	{
		if (av[i][0] == '|')
			*pipe_check = 1;
		if (av[i][0] == ';')
			return (i);
		i++;
	}
	return (i);
}

int	give_pipe_index(char **av, int start, int end)
{
	int	i;

	i = 0;
//	printf("i = %d\tstart = %d\n", i, start);
	while (i < start)
		i++;
	while (start < end)
	{
		if (av[start][0] == '|')
			return (i);
		i++;
		start++;
	}
	return (i);
}

void	refresh_fd(int count, int *p, int *n)
{
	if (count > 0)
	{
		close(*p);
		close(n[1]);
	}
	*p = n[0];
	close(n[1]);
}

int	count_p(char **av, int start, int end)
{
	int	count;

	count = 0;
	while (start < end)
	{
		if (av[start][0] == '|')
			count++;
		start++;
	}
	return (count);
}

void	execute_multiple_cmd(char **av, char **env, int start, int end)
{
	int	i;
	int	id;
	int	pipe_count;
	int	pipe_index;
	int old_fd = -1;
	int	new_fd[2];

	i = 0;
	pipe_count = count_p(av, start, end);
	while (i < pipe_count + 1 && av[start] != NULL)
	{
		pipe_index = give_pipe_index(av, start, end);
		if (pipe(new_fd) == -1)
		{
			write(2, "-error: fatal\n", 13);
			exit(EXIT_FAILURE);
		}
		id = fork();
		if (id == -1)
		{
			write(2, "*error: fatal\n", 13);
			exit(EXIT_FAILURE);
		}
		if (id == 0)
		{
			av[pipe_index] = NULL;
			if (i > 0)
			{
				if (dup2(old_fd, STDIN_FILENO) == -1)
				{
					write(2, "error: fatal\n", 13);
					exit(EXIT_FAILURE);
				}
				close(old_fd);
			}
			if (i < pipe_count)
			{
				close(new_fd[0]);
				if (dup2(new_fd[1], STDOUT_FILENO) == -1)
				{
					write(2, "+error: fatal\n", 13);
					exit(EXIT_FAILURE);
				}
				close(new_fd[1]);
			}
			if (execve(av[start], av + start, env) == -1)
			{
				write(2, "error: cannot execute ", 22);
				write(2, av[start], strlen(av[start]));
				write(2, "\n", 1);
				exit(EXIT_FAILURE);
			}
		}
		start = pipe_index + 1;
		refresh_fd(i, &old_fd, new_fd);
		i++;
		waitpid(0, 0, 0);
	}
	close(old_fd);
}

void	execute_cd_command(char **av, int start)
{
	if (av[start + 2] != NULL)
	{
		write(2, "error: cd: bad arguments\n", 25);
		exit(EXIT_FAILURE);
	}
	if (chdir(av[start + 1]) == -1)
	{
		write(2, "error: cd: cannot change directory to ", 39);
		write(2, av[start + 1], strlen(av[start + 1]));
		write(2, "\n", 1);
		exit(EXIT_FAILURE);
	}
}

void	execute_cmd(char **av, char **env, int start, int end)
{
	int	pid;

	if (strcmp(av[start], ";") == 0)
		return ;
	pid = fork();
	if (pid == -1)
	{
		write(2, "error: fatal\n", 13);
		exit(EXIT_FAILURE);
	}
	if (pid == 0)
	{
		if (strcmp(av[start], "cd") == 0)
			execute_cd_command(av, start);
		av[end] = NULL;
		if (execve(av[start], av + start, env) == -1)
		{
			
			write(2, "error: cannot execute ", 22);
			write(2, av[start], strlen(av[start]));
			write(2, "\n", 1);
			exit(EXIT_FAILURE);
		}
	}
	else
		waitpid(pid, NULL, 0);
}

int main(int ac, char **av, char **env)
{
	int	i;
	int	semi_colon_index;
	int	pipe_check;

	i = 1;
	while (i < ac)
	{
		semi_colon_index = give_semi_colon_pos(av, i, &pipe_check);
		if (pipe_check == 1)
			execute_multiple_cmd(av, env, i, semi_colon_index);
		else
			execute_cmd(av, env, i, semi_colon_index);
		i = semi_colon_index + 1;
	}
	return (0);
}

