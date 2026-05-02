async function loadChat() {
    try {
        const response = await fetch("history.txt");
        const text = await response.text();

        document.getElementById("chat-box").textContent = text;
    } catch (error) {
        document.getElementById("chat-box").textContent =
            "Could not load chat history.";
    }
}

loadChat();
setInterval(loadChat, 2000);