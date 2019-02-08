#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sem.h>

#define MAX 64

int main() {
	int fd1 = open("rendezvous1.txt", O_RDWR|O_CREAT, 0600);
	int fd2 = open("rendezvous2.txt", O_RDWR|O_CREAT, 0600);
	if (fd1 == -1 || fd2 == -1) {
		perror("could not create files");
		return 1;
	}

	key_t key = 123;
	int semid = semget(key, 2, IPC_CREAT|IPC_EXCL|0600);
	if (semid == -1) {
		perror("semget failure");
		return  1;
	}

	if (semctl(semid, 0, SETVAL, 0) == -1 || semctl(semid, 1, SETVAL, 1) == -1) {
		perror("semctl failure");
		return  1;
	}

	struct sembuf aWait = { 0, -1, 0 };
	struct sembuf aSignal = { 0, 1, 0 };
	struct sembuf bWait = { 1, -1, 0 };
	struct sembuf bSignal = { 1, 1, 0 };

	int pid = fork();
	if (pid > 0) {
		sleep(2);
		if (write(fd1, "ren", 3) == -1) { perror("write failure"); return 1; }
		if (semop(semid, &aSignal, 1) == -1) { perror("failure of semop"); return 1; }
		if (semop(semid, &bWait, 1) == -1) { perror("failure of semop"); return 1; }
		if (write(fd2, " me", 3) == -1) { perror("write failure"); return 1; }
	} else if (pid == 0) {
		if (write(fd2, "meet", 4) == -1) { perror("write failure"); return 1; }
		if (semop(semid, &bSignal, 1) == -1) { perror("failure of semop"); return 1; }
		if (semop(semid, &aWait, 1) == -1) { perror("failure of semop"); return 1; }
		if (write(fd1, "dezvous", 7)  == -1) { perror("write failure"); return 1; }
	} else {
		perror("fork failure");
		return 1;
	}

	if (pid > 0) {
		sleep(2);

		if (semctl(semid, 0, IPC_RMID, 0) == -1) {
			perror("Semaphore wasn't removed");
		}

		if (lseek(fd1, 0, SEEK_SET) == -1 || lseek(fd2, 0, SEEK_SET) == -1) {
			perror("lseek failure");
			return 1;
		}

		char string[MAX];

		if (read(fd1, string, 10) == -1) { perror("read failure"); return 1; }
		string[10] = '\n';
		if (write(STDOUT_FILENO, string, 11) == -1) { perror("write failure"); return 1; }

		if (read(fd2, string, 7) == -1) { perror("read failure"); return 1; }
		string[7] = '\n';
		if (write(STDOUT_FILENO, string, 8) == -1) { perror("write failure"); return 1; }

		if (close(fd1) == -1 || close(fd2) == -1) { perror("error closing files"); }
		if (unlink("rendezvous1.txt") == -1 || unlink("rendezvous2.txt") == -1) { perror("error unlinking files"); }
	}
	return 0;
}