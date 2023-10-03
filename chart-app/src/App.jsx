import React, {useEffect, useState} from 'react';
import './App.css';
import {LineChart, XAxis, YAxis, Line, Legend, ResponsiveContainer, ReferenceLine, Brush} from 'recharts';
import axios from 'axios';
import _ from 'lodash';

const deadline = 2; // Scadenza di 2 secondi

function App() {
    const [data, setData] = useState([]);
    const [ids, setIds] = useState([]);

    useEffect(() => {
        const fetchData = async () => {
            console.log('Fetching data...');
            try {
                const result = await axios.get('http://127.0.0.1:2000/data');

                // check if the response is ok
                if (result.status !== 200) {
                    console.error('Error while fetching data');
                    return;
                }

                const messages = result.data.messages;

                // Calculate the arrival time for each message and format the data
                const formattedData = messages.map((message) => ({
                    id: message.id,
                    arrival_time: (message.recv_time - message.send_time) / 1000, // Convert to seconds
                }));

                // Data Sampling for improving performance
                const sampledData = _.sampleSize(formattedData, 100); // Esempio: riduci a 100 punti dati

                // Add new ids to the list of ids
                const newIds = messages.map((message) => message.id);
                setIds((prevIds) => [...prevIds, ...newIds]);

                // Combine the new data with the old one
                setData((prevData) => [...prevData, ...sampledData]);
            } catch (error) {
                console.error('Error while fetching data:', error);
            }
        };

        const fetchInterval = setInterval(fetchData, 5000); // Esegui il fetch ogni 5 secondi

        return () => {
            clearInterval(fetchInterval); // Clear interval on unmount
        };
    }, [ids]);

    return (
        <div className="App">
            <h1>Realtime Chart</h1>
            <ResponsiveContainer width="100%" height={700}>
                <LineChart data={data}>
                    <XAxis dataKey="id"/>
                    <YAxis/>
                    <Legend/>
                    <Line
                        dataKey="arrival_time"
                        name="Tempo di arrivo"
                        type="monotone"
                        stroke="#8884d8"
                    />
                    <ReferenceLine y={deadline} stroke="red" label="Deadline"/>
                    <Brush dataKey="id" height={30} stroke="#8884d8"/>
                </LineChart>
            </ResponsiveContainer>
        </div>
    );
}

export default App;
