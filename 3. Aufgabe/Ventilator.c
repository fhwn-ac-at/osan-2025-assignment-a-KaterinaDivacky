//Divacky Katerina
//Task Ventilator

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <mqueue.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_TASKS 100 //Max. Anzahl an Tasks
#define MAX_QUEUE_NAME 64 // Max. Länge von Queue-Namens
#define MAX_MSG_SIZE sizeof(Task) //Größe der Nachricht in Task-Queue
#define MAX_STAT_SIZE sizeof(WorkerStats) //Größe der Nachricht in Result-Queue
#define TERMINATE -1 //Terminierungsnachricht Kennzeichnung

typedef struct
{
    int effort; //Aufwand in Sek. des Tasks
} Task;

typedef struct
{
    int worker_id;
    pid_t pid;
    int tasks_processed;
    int total_time;
} WorkerStats;

//Gibt formatierten Zeitstempel im "HH:MM:SS |" zurück
char* timestamp()
{
    static char buffer[20];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d |", t->tm_hour, t->tm_min, t->tm_sec);
    return buffer;
}

//Ausführung von Worker-Prozess
void worker_process(const char *task_queue_name, const char *completion_queue_name, int id)
{
    mqd_t task_q = mq_open(task_queue_name, O_RDONLY);
    mqd_t result_q = mq_open(completion_queue_name, O_WRONLY);

    if (task_q == (mqd_t)-1 || result_q == (mqd_t)-1)
    {
        printf("%s Worker #%02d | Failed to open queues\n", timestamp(), id);
        exit(1);
    }

    printf("%s Worker #%02d | Started worker PID %d\n", timestamp(), id, getpid());

    Task task;
    WorkerStats stats = {id, getpid(), 0, 0};

    while (1)
    {
        ssize_t bytes_read = mq_receive(task_q, (char*)&task, MAX_MSG_SIZE, NULL);
        if (bytes_read >= 0)
        {
            //Prüfen auf Termination
            if (task.effort == TERMINATE)
            {
                printf("%s Worker #%02d | Received termination task\n", timestamp(), id);
                break;
            }
            //Task empfangen und verarbeiten
            printf("%s Worker #%02d | Received task with effort %d\n", timestamp(), id, task.effort);
            sleep(task.effort);
            stats.tasks_processed++;
            stats.total_time += task.effort;
        }
        else if (errno != EINTR)
        {
            perror("mq_receive");
            break;
        }
    }

    //Statistiken an den Ventilator zurücksenden
    mq_send(result_q, (const char*)&stats, MAX_STAT_SIZE, 0);
    mq_close(task_q);
    mq_close(result_q);
    exit(0);
}

int main(int argc, char *argv[])
{
    int num_workers = 2, num_tasks = 10, queue_size = 5;
    int opt;

    //Kommandozeilenargumente parsen
    while ((opt = getopt(argc, argv, "w:t:s:")) != -1)
    {
        switch (opt)
        {
            case 'w': num_workers = atoi(optarg); break;
            case 't': num_tasks = atoi(optarg); break;
            case 's': queue_size = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -w <workers> -t <tasks> -s <queue_size>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    //Namen für Message Queues erzeugen
    char task_queue_name[MAX_QUEUE_NAME];
    char result_queue_name[MAX_QUEUE_NAME];
    snprintf(task_queue_name, MAX_QUEUE_NAME, "/task_queue_%d", getpid());
    snprintf(result_queue_name, MAX_QUEUE_NAME, "/completion_queue_%d", getpid());

    //Alte Queues entfernen
    mq_unlink(task_queue_name);
    mq_unlink(result_queue_name);

    //Attribute für Queues setzen
    struct mq_attr task_attr = {.mq_flags = 0, .mq_maxmsg = queue_size, .mq_msgsize = MAX_MSG_SIZE, .mq_curmsgs = 0};
    struct mq_attr stat_attr = {.mq_flags = 0, .mq_maxmsg = num_workers, .mq_msgsize = MAX_STAT_SIZE, .mq_curmsgs = 0};

    //Queues erstellen
    mqd_t task_q = mq_open(task_queue_name, O_CREAT | O_RDWR, 0644, &task_attr);
    mqd_t result_q = mq_open(result_queue_name, O_CREAT | O_RDWR, 0644, &stat_attr);

    if (task_q == (mqd_t)-1 || result_q == (mqd_t)-1)
    {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    printf("%s Ventilator | Starting %d workers for %d tasks and a queue size of %d\n",
           timestamp(), num_workers, num_tasks, queue_size);

    //Worker-Prozesse erzeugen
    pid_t worker_pids[num_workers];
    for (int i = 0; i < num_workers; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            //Kindprozess führt Worker-Code aus
            worker_process(task_queue_name, result_queue_name, i + 1);
        }
        else if (pid > 0)
        {
            //Parent speichert PID für späteres wait
            worker_pids[i] = pid;
        }
        else
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    //Aufgaben generieren und in Queue einfügen
    printf("%s Ventilator | Distributing tasks\n", timestamp());
    srand(time(NULL));

    for (int i = 0; i < num_tasks; i++)
    {
        Task task = {.effort = rand() % 10 + 1}; //Zufälliger Aufwand 1–10
        printf("%s Ventilator | Queuing task #%d with effort %d\n", timestamp(), i + 1, task.effort);
        if (mq_send(task_q, (const char*)&task, MAX_MSG_SIZE, 0) == -1)
        {
            perror("mq_send");
        }
    }

    //Terminierungsaufgaben an Worker senden
    printf("%s Ventilator | Sending termination tasks\n", timestamp());
    for (int i = 0; i < num_workers; i++)
    {
        Task terminate = {.effort = TERMINATE};
        mq_send(task_q, (const char*)&terminate, MAX_MSG_SIZE, 0);
    }

    //Statistiken von den Workern einsammeln
    printf("%s Ventilator | Waiting for workers to terminate\n", timestamp());

    for (int i = 0; i < num_workers; i++)
    {
        WorkerStats stats;
        ssize_t bytes_read = mq_receive(result_q, (char*)&stats, MAX_STAT_SIZE, NULL);
        if (bytes_read >= 0)
        {
            printf("%s Ventilator | Worker %d processed %d tasks in %d seconds\n",
                   timestamp(), stats.worker_id, stats.tasks_processed, stats.total_time);
        }
        else
        {
            perror("mq_receive (completion)");
        }
    }

    //Auf Kinder warten und Exit-Status ausgeben
    for (int i = 0; i < num_workers; i++)
    {
        int status;
        pid_t pid = waitpid(worker_pids[i], &status, 0);
        if (WIFEXITED(status))
        {
            printf("%s Ventilator | Worker %d with PID %d exited with status %d\n",
                   timestamp(), i + 1, pid, WEXITSTATUS(status));
        }
    }

    //Queues schließen und löschen
    mq_close(task_q);
    mq_unlink(task_queue_name);
    mq_close(result_q);
    mq_unlink(result_queue_name);

    return 0;
}
