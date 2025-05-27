"use client";
import React, { useState } from 'react';

export default function Home() {
    const [url, setUrl] = useState('');
    const [responseData, setResponseData] = useState(null);
    const [cacheData, setCacheData] = useState([]);

    const handleSubmit = async (e) => {
        e.preventDefault();
        try {
            const res = await fetch('http://localhost:5000/fetch', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ url })
            });
            const result = await res.json();
            console.log(result);
            setResponseData({
              Thread_ID: result.thread,
              Cache_Hit: result.cache ? 'Yes' : 'No',
              Data: result.data
            });
setCacheData(result.cache_state || []);
        } catch (err) {
            setResponseData({ error: err.message });
            setCacheData([]);
        }
    };

    return (
        <div style={styles.container}>
            <h1 style={styles.title}>🛡️ Proxy Server Dashboard</h1>

            <form onSubmit={handleSubmit} style={styles.form}>
                <input
                    type="text"
                    value={url}
                    onChange={(e) => setUrl(e.target.value)}
                    placeholder="Enter URL"
                    style={styles.input}
                />
                <button type="submit" style={styles.button}>Fetch</button>
            </form>

            <div style={styles.sectionWrapper}>
                {/* Response Section */}
                <div style={styles.panel}>
                    <h2 style={styles.panelTitle}>📦 Response Data</h2>
                    {responseData ? (
                        <table style={styles.table}>
                            <thead>
                                <tr>
                                    <th style={styles.th}>Key</th>
                                    <th style={styles.th}>Value</th>
                                </tr>
                            </thead>
                            <tbody>
                                {Object.entries(responseData).map(([key, value]) => (
                                    <tr key={key}>
                                        <td style={styles.td}>{key}</td>
                                        <td style={styles.td}>
                                            {typeof value === 'object'
                                                ? <pre style={styles.pre}>{JSON.stringify(value, null, 2)}</pre>
                                                : value?.toString()}
                                        </td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    ) : (
                        <p style={styles.placeholder}>No response fetched yet.</p>
                    )}
                </div>

                {/* Cache Section */}
                <div style={styles.panel}>
                    <h2 style={styles.panelTitle}>🧠 Cache State</h2>
                    {cacheData.length > 0 ? (
                        <table style={styles.table}>
                            <thead>
                                <tr>
                                    <th style={styles.th}>#</th>
                                    <th style={styles.th}>URL</th>
                                    <th style={styles.th}>Size</th>
                                    <th style={styles.th}>Score</th>
                                    <th style={styles.th}>Freq</th>
                                </tr>
                            </thead>
                            <tbody>
                                {cacheData.map((item, index) => (
                                    <tr key={index}>
                                        <td style={styles.td}>{index + 1}</td>
                                        <td style={styles.td}>{item.url}</td>
                                        <td style={styles.td}>{item.size}</td>
                                        <td style={styles.td}>{item.score}</td>
                                        <td style={styles.td}>{item.freq}</td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    ) : (
                        <p style={styles.placeholder}>Cache is empty.</p>
                    )}
                </div>
            </div>
        </div>
    );
}

const neon = "#00ff00";

const styles = {
    container: {
        backgroundColor: '#0a0a0a',
        color: neon,
        minHeight: '100vh',
        padding: '2rem',
        fontFamily: 'Courier New, monospace',
    },
    title: {
        fontSize: '2rem',
        textAlign: 'center',
        marginBottom: '2rem',
        textShadow: `0 0 5px ${neon}`,
    },
    form: {
        display: 'flex',
        justifyContent: 'center',
        gap: '1rem',
        marginBottom: '2rem',
    },
    input: {
        backgroundColor: 'black',
        color: neon,
        border: `1px solid ${neon}`,
        padding: '10px',
        width: '60%',
        fontSize: '1rem',
    },
    button: {
        backgroundColor: 'black',
        color: neon,
        border: `1px solid ${neon}`,
        padding: '10px 20px',
        cursor: 'pointer',
        transition: 'all 0.2s ease-in-out',
    },
    sectionWrapper: {
        display: 'flex',
        gap: '2rem',
        flexWrap: 'wrap',
    },
    panel: {
        flex: '1 1 45%',
        border: `1px solid ${neon}`,
        padding: '1rem',
        borderRadius: '10px',
        backgroundColor: '#111',
        boxShadow: `0 0 10px ${neon}`,
    },
    panelTitle: {
        borderBottom: `1px solid ${neon}`,
        marginBottom: '1rem',
        paddingBottom: '0.5rem',
    },
    table: {
        width: '100%',
        borderCollapse: 'collapse',
    },
    th: {
        border: `1px solid ${neon}`,
        padding: '8px',
        backgroundColor: '#0f0f0f',
        textAlign: 'left',
    },
    td: {
        border: `1px solid ${neon}`,
        padding: '8px',
        verticalAlign: 'top',
        wordBreak: 'break-word',
    },
    pre: {
        whiteSpace: 'pre-wrap',
        margin: 0,
    },
    placeholder: {
        color: '#888',
        fontStyle: 'italic',
    },
};
