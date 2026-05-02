cat > ~/mpi_chat/chat.c << 'EOF'
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_MSG 200

typedef struct {
    int sender;
    int receiver;
    char text[MAX_MSG];
} Message;

void save_history(Message msg) {
    FILE *file = fopen("history.txt", "a");
    if (file != NULL) {
        fprintf(file, "[%d -> %d]: %s\n",
                msg.sender, msg.receiver, msg.text);
        fclose(file);
    }
}

void show_history() {
    FILE *file = fopen("history.txt", "r");
    char line[300];

    if (file == NULL) {
        printf("No chat history found.\n");
        return;
    }

    printf("\n===== CHAT HISTORY =====\n");

    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);
    printf("========================\n");
}

int main(int argc, char *argv[]) {
    int rank, size;
    int choice;
    Message msg;
    MPI_Status status;
    char buf[20];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    while (1) {

        if (rank == 0) {
            printf("\n===== MPI CHAT APP =====\n");
            printf("1. Send Message\n");
            printf("2. Show Chat History\n");
            printf("3. Exit\n");
            printf("4. Clear History\n");
            printf("Choice: ");
            fflush(stdout);

            fgets(buf, sizeof(buf), stdin);
            sscanf(buf, "%d", &choice);

            if (choice == 1) {
                printf("Sender (1-%d): ", size - 1);
                fflush(stdout);
                fgets(buf, sizeof(buf), stdin);
                sscanf(buf, "%d", &msg.sender);

                printf("Receiver (1-%d): ", size - 1);
                fflush(stdout);
                fgets(buf, sizeof(buf), stdin);
                sscanf(buf, "%d", &msg.receiver);

                if (msg.sender <= 0 || msg.sender >= size ||
                    msg.receiver <= 0 || msg.receiver >= size) {
                    printf("Invalid sender or receiver.\n");
                    continue;
                }

                printf("Message: ");
                fflush(stdout);
                fgets(msg.text, MAX_MSG, stdin);
                msg.text[strcspn(msg.text, "\n")] = 0;

                MPI_Send(&msg, sizeof(Message), MPI_BYTE,
                         msg.receiver, 0, MPI_COMM_WORLD);

                save_history(msg);

                printf("Delivered: [%d -> %d]: %s\n",
                       msg.sender, msg.receiver, msg.text);
            }

            else if (choice == 2) {
                show_history();
            }

            else if (choice == 3) {
                strcpy(msg.text, "exit");

                for (int i = 1; i < size; i++) {
                    MPI_Send(&msg, sizeof(Message), MPI_BYTE,
                             i, 0, MPI_COMM_WORLD);
                }
                break;
            }

            else if (choice == 4) {
                FILE *f = fopen("history.txt", "w");
                if (f) { fclose(f); }
                printf("History cleared.\n");
            }

            else {
                printf("Invalid choice.\n");
            }
        }

        else {
            MPI_Recv(&msg, sizeof(Message), MPI_BYTE,
                     0, 0, MPI_COMM_WORLD, &status);

            if (strcmp(msg.text, "exit") == 0)
                break;

            fflush(stdout);
            printf("\n[Process %d received from %d]: %s\n",
                   rank, msg.sender, msg.text);
            fflush(stdout);
        }
    }

    MPI_Finalize();
    return 0;
}
EOF
