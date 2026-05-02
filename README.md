# MPI-Based Chat Application

A parallel computing project demonstrating inter-process communication using the Message Passing Interface (MPI) in C.

---

## 📝 Project Overview
This application simulates a centralized chat system where a **Master Process (Rank 0)** manages user inputs and routes messages to **Client Processes (Ranks 1-N)**.

### Architecture
- **Controller (Rank 0):** Displays the menu, captures user input, saves history to disk, and routes messages.
- **Users (Rank 1+):** Standby processes that listen for and display incoming messages.

---

## 🛠️ Technologies Used
- **C Language**
- **OpenMPI** (Message Passing Interface)
- **WSL (Ubuntu)**
- **Visual Studio Code**

---

## 📂 Project Structure
- `chat.c`: The main source code containing MPI logic and file handling.
- `chat`: The compiled executable binary.
- `history.txt`: Automatically generated file storing all chat logs.

---

## 🚀 How to Setup and Run

### 1. Switch to User
Avoid running as root. Switch to your standard user:
```bash

su - sarim
2. Navigate to Directory
cd ~/mpi_chat
3. Compile the Code
Use the mpicc wrapper to link MPI libraries correctly:
mpicc chat.c -o chat
4. Run the Application
Launch the cluster with 4 processes (adjust -np as needed):
mpirun --oversubscribe -np 4 ./chat
