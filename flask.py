from apscheduler.schedulers.background import BackgroundScheduler
from flask import Flask
import time

traffic = {
    "id" :"1",
    "description" : "사거리",
    "signal" :0,
    "time": 0,
}
#signal = 14 is red
#singal = 15 is yellow
#signal = 16 is green

#-----
#스케줄 실행 코드

def scheduler():
    if traffic['signal'] == 14:
        for i in range(6,-1,-1):
            time.sleep(1)
            traffic['time']=i
        traffic['time']=5
        traffic['signal'] = 16
    elif traffic['signal'] == 15:
        traffic['signal'] = 14

        traffic['time']=0
    else:
        traffic['signal']=16
        for i in range(5,-1,-1):
            time.sleep(1)
            traffic['time']=i
        traffic['signal'] = 15

schedule = BackgroundScheduler(daemon=True, timezone='Asia/Seoul')
schedule.add_job(scheduler, 'interval', seconds=3)
schedule.start()
#-----


#-----
#flask 서버 실행 코드
app = Flask(__name__)

@app.route("/", methods=["GET"])
def ping():
    return traffic

if __name__ == '__main__':
    app.debug = True
    app.run(host="0.0.0.0",port=80)
#-----