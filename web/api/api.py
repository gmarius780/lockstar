from flask import Flask
from flask import request

app = Flask(__name__)

@app.route('/api', methods=['GET'])
def api():
    return {
        'userId': 1,
        'title': 'Flask React Application',
        'completed': False
    }

@app.route("/pid", methods=["POST"], strict_slashes=False)
def add_articles():
    
    p = request.json['p']
    i = request.json['i']
    d = request.json['d']

    print("recieved_vals: p-", p,"i-", i,"d-", d)

    return {
        'p': p,
        'i': i,
        'd': d
    }

    # return 'Done', 201