from flask import Flask, request, jsonify
from flask_cors import CORS
import socket
import json

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

PROXY_HOST = 'localhost'
PROXY_PORT = 8080

@app.route('/fetch', methods=['POST'])
def fetch_url():
    data = request.get_json()
    url = data.get('url')
    
    if not url:
        return jsonify({"error": "URL not provided"}), 400

    try:
        # Create proper HTTP request
        http_req = f"GET {url} HTTP/1.1\r\nHost: {url.split('/')[2]}\r\n\r\n"
        
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)
            s.connect((PROXY_HOST, PROXY_PORT))
            s.sendall(http_req.encode())
            
            response = b""
            while True:
                chunk = s.recv(4096)
                if not chunk: break
                response += chunk
                
        try:
            return jsonify(json.loads(response.decode()))
        except json.JSONDecodeError:
            return jsonify({
                "error": "Invalid response from proxy",
                "raw_response": response.decode(errors='replace')
            }), 500

    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)