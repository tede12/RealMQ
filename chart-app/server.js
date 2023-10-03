import express from 'express';
import fs from 'fs';
import cors from 'cors'; // Importa il modulo cors

const app = express();
const port = 2000;

// Abilita CORS per tutte le route
app.use(cors());

app.get('/data', (req, res) => {
    fs.readFile('../server_stats.json', 'utf8', (err, data) => {
        if (err) {
            res.status(500).json({error: 'Errore nella lettura del file JSON'});
            return;
        }

        try {
            const jsonData = JSON.parse(data);
            res.json(jsonData);
        } catch (err) {
            res.status(500).json({error: 'Errore nel parsing del file JSON'});
        }
    });
});

app.listen(port, () => {
    console.log(`Server listening on port ${port}`);
});
