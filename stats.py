import json

data = json.load(open('server_stats.json', 'r'))
# check how many messages are out of the deadline time (2 seconds)
deadline = 2
messages_out_of_time = 0
for message in data['messages']:    #
    if (message['recv_time'] - message['send_time']) > deadline:
        messages_out_of_time += 1

print(f"Messages out of time: {messages_out_of_time}, Total messages: {len(data['messages'])}")
print(f"Percentage of messages out of time: {messages_out_of_time / len(data['messages']) * 100}%")
