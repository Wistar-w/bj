import requests
import time
import random

# 你的 cookies
cookies = {
    'bzb_njw': '8BC063B7625F43AA658724DD80BB0308',
    'JSESSIONID': '1EA52AA30BC0D6ED08A8CAEAFFDDA4B4',
    'SERVERID': '224',
}

url = 'https://jwmis.hhtc.edu.cn/jsxsd/xsxkkc/ggxxkxkOper?kcid=B99489C9F3934F9DACA0703A59AF893F&cfbs=null&jx0404id=202520262005777&xkzy=&trjf='
# 设置最大尝试次数
max_attempts = 10

for attempt in range(1, max_attempts + 1):
    try:
        print(f"第 {attempt} 次尝试...")
        # 不设置 headers，仅携带 cookies
        resp = requests.get(url, cookies=cookies, timeout=5)
        resp.raise_for_status()

        print(resp.text[:200])  # 打印前200字符

        # 根据实际返回内容判断是否成功
        if "成功" in resp.text or "success" in resp.text:
            print("选课成功！停止请求。")
            break

    except requests.exceptions.RequestException as e:
        print(f"请求出错：{e}")
        break

    # 随机休眠 1~3 秒
    time.sleep(random.uniform(1, 3))
