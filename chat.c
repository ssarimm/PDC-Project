#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_MSG      200
#define MAX_HISTORY  100   // max messages stored in history

#define TAG_NOOP 0
#define TAG_CHAT 1
#define TAG_EXIT 2
#define TAG_ACK  3

typedef struct {
    int sender;
    int receiver;   // -1 means broadcast
    char text[MAX_MSG];
    char timestamp[32];
} Message;

// ── History (rank 0 only) ──────────────────────────────────────────────────
typedef struct {
    char type[12];          //private or broadcast
    int  sender;
    int  receiver;          // -1 for broadcast
    char text[MAX_MSG];
    char timestamp[32];
} HistoryEntry;

static HistoryEntry history[MAX_HISTORY];
static int          history_count = 0;

void history_add(const char *type, int sender, int receiver,
                 const char *text, const char *timestamp) {
    if (history_count >= MAX_HISTORY) {
        // Shift oldest entry out
        memmove(&history[0], &history[1],
                sizeof(HistoryEntry) * (MAX_HISTORY - 1));
        history_count = MAX_HISTORY - 1;
    }
    HistoryEntry *e = &history[history_count++];
    strncpy(e->type,      type,      sizeof(e->type)      - 1);
    strncpy(e->text,      text,      sizeof(e->text)      - 1);
    strncpy(e->timestamp, timestamp, sizeof(e->timestamp) - 1);
    e->sender   = sender;
    e->receiver = receiver;
}

void history_print() {
    printf("\n========== CHAT HISTORY (%d message%s) ==========\n",
           history_count, history_count == 1 ? "" : "s");
    if (history_count == 0) {
        printf("  (no messages yet)\n");
    }
    for (int i = 0; i < history_count; i++) {
        HistoryEntry *e = &history[i];
        if (e->receiver == -1) {
            printf("  [%d] [%s] [BROADCAST] P%d → ALL  : %s\n",
                   i + 1, e->timestamp, e->sender, e->text);
        } else {
            printf("  [%d] [%s] [PRIVATE  ] P%d → P%d : %s\n",
                   i + 1, e->timestamp, e->sender, e->receiver, e->text);
        }
    }
    printf("=================================================\n");
}


void clear_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void get_timestamp(char *buffer, size_t len) {
    time_t now = time(NULL);
    strftime(buffer, len, "%H:%M:%S", localtime(&now));
}


int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Status status;
    Message msg;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    while (1) {
        memset(&msg, 0, sizeof(Message));
        int choice = 0;

        if (rank == 0) {
            printf("\n===== MPI CHAT MENU =====\n");
            printf("1. Send Private Message\n");
            printf("2. Broadcast Message\n");
            printf("3. Show Chat History\n");
            printf("4. Exit\n");
            printf("Choice: ");
            fflush(stdout);

            if (scanf("%d", &choice) != 1) {
                clear_buffer();
                printf("Invalid input.\n");
                choice = 0;
            } else {
                clear_buffer();
            }

            //private message
            if (choice == 1) {
                printf("Sender ID (1-%d): ", size - 1);
                fflush(stdout);
                if (scanf("%d", &msg.sender) != 1) {
                    clear_buffer(); printf("Invalid sender.\n"); choice = 0;
                } else { clear_buffer(); }

                if (choice != 0) {
                    printf("Receiver ID (1-%d): ", size - 1);
                    fflush(stdout);
                    if (scanf("%d", &msg.receiver) != 1) {
                        clear_buffer(); printf("Invalid receiver.\n"); choice = 0;
                    } else { clear_buffer(); }
                }

                if (choice != 0) {
                    if (msg.sender < 1 || msg.sender >= size ||
                        msg.receiver < 1 || msg.receiver >= size ||
                        msg.sender == msg.receiver) {
                        printf("Invalid sender/receiver IDs.\n");
                        choice = 0;
                    }
                }

                if (choice != 0) {
                    printf("Enter Message: ");
                    fflush(stdout);
                    fgets(msg.text, MAX_MSG, stdin);
                    msg.text[strcspn(msg.text, "\n")] = '\0';
                    if (strlen(msg.text) == 0) {
                        printf("Empty message not allowed.\n"); choice = 0;
                    } else {
                        get_timestamp(msg.timestamp, sizeof(msg.timestamp));
                    }
                }
            }

            //broadcast
            else if (choice == 2) {
                printf("Sender ID (1-%d): ", size - 1);
                fflush(stdout);
                if (scanf("%d", &msg.sender) != 1) {
                    clear_buffer(); printf("Invalid sender.\n"); choice = 0;
                } else { clear_buffer(); }

                if (choice != 0) {
                    if (msg.sender < 1 || msg.sender >= size) {
                        printf("Invalid sender ID.\n"); choice = 0;
                    }
                }

                if (choice != 0) {
                    msg.receiver = -1;
                    printf("Enter Message: ");
                    fflush(stdout);
                    fgets(msg.text, MAX_MSG, stdin);
                    msg.text[strcspn(msg.text, "\n")] = '\0';
                    if (strlen(msg.text) == 0) {
                        printf("Empty message not allowed.\n"); choice = 0;
                    } else {
                        get_timestamp(msg.timestamp, sizeof(msg.timestamp));
                    }
                }
            }

            //show histrory
            else if (choice == 3) {
                history_print();
                // No workers need to be notified — history is local to rank 0.
                // Fall through to dispatch loop which will send NOOP to all.
                choice = 0;  
            }

            //exit
            else if (choice == 4) {
                for (int i = 1; i < size; i++) {
                    MPI_Send(&msg, sizeof(Message), MPI_BYTE,
                             i, TAG_EXIT, MPI_COMM_WORLD);
                }
                printf("Exiting chat...\n");
                break;
            }

            else {
                if (choice != 0) printf("Invalid choice.\n");
                choice = 0;
            }

            //dispatch to all workers
            int acks_expected = 0;

            for (int i = 1; i < size; i++) {
                if (choice == 1 && i == msg.receiver) {
                    MPI_Send(&msg, sizeof(Message), MPI_BYTE,
                             i, TAG_CHAT, MPI_COMM_WORLD);
                    acks_expected++;
                }
                else if (choice == 2 && i != msg.sender) {
                    MPI_Send(&msg, sizeof(Message), MPI_BYTE,
                             i, TAG_CHAT, MPI_COMM_WORLD);
                    acks_expected++;
                }
                else {
                    MPI_Send(&msg, sizeof(Message), MPI_BYTE,
                             i, TAG_NOOP, MPI_COMM_WORLD);
                }
            }

            // Record in history after dispatch (only successful sends)
            if (choice == 1) {
                history_add("PRIVATE", msg.sender, msg.receiver,
                            msg.text, msg.timestamp);
            } else if (choice == 2) {
                history_add("BROADCAST", msg.sender, -1,
                            msg.text, msg.timestamp);
            }

            // Wait for all receivers to ack before reprinting menu
            for (int i = 0; i < acks_expected; i++) {
                char ack;
                MPI_Recv(&ack, 1, MPI_BYTE,
                         MPI_ANY_SOURCE, TAG_ACK, MPI_COMM_WORLD, &status);
            }

            if (acks_expected > 0) printf("\n");
        }

        //worker
        else {
            MPI_Recv(&msg, sizeof(Message), MPI_BYTE,
                     0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TAG_EXIT) break;

            if (status.MPI_TAG == TAG_CHAT) {
                if (msg.receiver == -1) {
                    printf("[BROADCAST] [P%d received from P%d at %s]: %s\n",
                           rank, msg.sender, msg.timestamp, msg.text);
                } else {
                    printf("[PRIVATE]   [P%d received from P%d at %s]: %s\n",
                           rank, msg.sender, msg.timestamp, msg.text);
                }
                fflush(stdout);

                char ack = 1;
                MPI_Send(&ack, 1, MPI_BYTE, 0, TAG_ACK, MPI_COMM_WORLD);
            }
            // TAG_NOOP: nothing to do
        }
    }

    MPI_Finalize();
    return 0;
}
