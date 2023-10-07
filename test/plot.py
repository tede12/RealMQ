import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import zmq
import json

# Inizializza il contesto ZeroMQ
context = zmq.Context()

# Crea un socket SUB per ricevere i dati
socket = context.socket(zmq.SUB)
socket.connect("tcp://127.0.0.1:5565")
socket.setsockopt(zmq.SUBSCRIBE, b"")

# Inizializza le liste per i dati
x_data = []
y_data = []

# Crea una figura e assi
fig, ax = plt.subplots()

# Imposta i label degli assi e il titolo
ax.set_xlabel('Asse X')
ax.set_ylabel('Asse Y')
ax.set_title('Grafico dei dati di ZeroMQ')


# Funzione di aggiornamento per FuncAnimation
def update(frame):
    global x_data, y_data

    data = socket.recv()
    try:
        json_data = json.loads(data)  # Analizza la stringa JSON in un dizionario Python
        x_value = json_data['count']  # Estrai il valore di 'x' dal dizionario
        y_value = json_data['rtt']  # Estrai il valore di 'y' dal dizionario
        # Aggiungi i dati alle liste
        x_data.append(x_value)
        y_data.append(y_value)

        # Pulisci gli assi e ridisegna il grafico con i nuovi dati
        ax.clear()
        ax.plot(x_data, y_data)

        # Imposta di nuovo i label e il titolo
        ax.set_xlabel('Asse X')
        ax.set_ylabel('Asse Y')
        ax.set_title('Grafico dei dati di ZeroMQ')
    except json.JSONDecodeError:
        pass


# Usa FuncAnimation per aggiornare il grafico
ani = FuncAnimation(fig, update, frames=None, repeat=True)

# Mostra il grafico
plt.show()
