#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
deepseek_proxy.py — DeepSeek 用量仪表盘的局域网数据代理
========================================================
作用：给 STM32 开发板喂"DeepSeek 用量页"数据。
  - 板子无法直连 HTTPS（LwIP 无 TLS、无 DNS），因此由本脚本做 HTTPS/TLS + API Key + 记账，
    板子只通过局域网明文 HTTP 拉 `GET /api/dashboard`。
  - 余额来自官方公开接口 `GET https://api.deepseek.com/user/balance`。
  - 用量/历史来自本地记账库（deepseek_usage.json）；`--seed-demo` 填充演示数据保证首屏不空。

用法：
  export DEEPSEEK_API_KEY=sk-xxx
  python tools/deepseek_proxy.py --host 0.0.0.0 --port 8000 --seed-demo
  (脚本启动后打印本地局域网 IP 与端口，把它填到 app/services/ai_dash_api.h 的 AIDASH_PROXY_IP)

接口：
  GET  /api/dashboard   -> 扁平行式文本（板端无需 JSON 解析）
  POST /log             -> 注入一条合成 usage（body: JSON，含 model/prompt_tokens/completion_tokens 等）
  GET  /                -> 极简 HTML 自检页

定价模型（可按官方最新对价调整，单位 USD / 1M tokens）：
  高峰时段（北京时间 09-12、14-18）费用为 PEAK_FACTOR 倍。
"""

import json
import os
import socket
import threading
import time
from datetime import datetime, timezone, timedelta
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    import requests
except ImportError:
    requests = None  # 运行时检测并提示

DEEPSEEK_URL = "https://api.deepseek.com/user/balance"
API_KEY = os.environ.get("DEEPSEEK_API_KEY", "")
DB_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "deepseek_usage.json")

# ---- 计价参数（USD / 1M tokens；可自行按官方对价修改） ----
#      key             (in_miss, in_cache, out)
RATES_USD = {
    "deepseek-chat":    (0.27, 0.07, 1.10),
    "deepseek-reasoner": (0.55, 0.14, 2.19),
}
RATES_CNY_PER_USD = 7.2          # 仅做展示换算（代理不联网查汇率）
PEAK_FACTOR = 2.0                # 高峰计费倍数
PEAK_HOURS = {9, 10, 11, 14, 15, 16, 17}   # 北京 09-12、14-18 为高峰
CNY_SYMBOL = "¥"
USD_SYMBOL = "$"
SERIES_DAYS = 30                 # 折线/柱状图天数


# --------------------------------------------------------------------------- #
#  记账库                                                                      #
# --------------------------------------------------------------------------- #
def load_db():
    if os.path.exists(DB_FILE):
        try:
            with open(DB_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except (json.JSONDecodeError, OSError):
            pass
    return {"records": []}


def save_db(db):
    with open(DB_FILE, "w", encoding="utf-8") as f:
        json.dump(db, f, ensure_ascii=False)


def is_peak(ts_secs):
    """ts_secs: 秒级时间戳。返回当前是否北京高峰计费时段。"""
    t = datetime.fromtimestamp(ts_secs, tz=timezone(timedelta(hours=8)))  # 北京 = UTC+8
    return t.hour in PEAK_HOURS


def record_cost_usd(rec):
    model = rec.get("model", "deepseek-chat")
    in_miss, in_cache, out = RATES_USD.get(model, RATES_USD["deepseek-chat"])
    p_tok = rec.get("prompt_tokens", 0)
    c_tok = rec.get("cache_hit_tokens", rec.get("prompt_cache_hit_tokens", 0))
    out_tok = rec.get("completion_tokens", 0)
    miss = max(0, p_tok - c_tok)
    base = (miss * in_miss + c_tok * in_cache + out_tok * out) / 1e6
    return base * (PEAK_FACTOR if is_peak(rec.get("ts", int(time.time()))) else 1.0)


def seed_demo(db):
    """填充连续 SERIES_DAYS 天的演示数据，让图表首屏不空。"""
    if db["records"]:
        return
    now = int(time.time())
    day = 86400
    for d in range(SERIES_DAYS, 0, -1):
        ts = now - d * day
        # 白天多一些、晚上少一些，制造趋势
        base = 60 + int(40 * pow(1 + (SERIES_DAYS - d) / 30.0, 1.5))
        for _i in range(base):
            n = dict(
                ts=ts,
                model="deepseek-chat" if _i % 7 else "deepseek-reasoner",
                prompt_tokens=400 + (_i % 5) * 120,
                cache_hit_tokens=200 + (_i % 5) * 80,
                completion_tokens=120 + (_i % 4) * 60,
            )
            db["records"].append(n)
    save_db(db)


# --------------------------------------------------------------------------- #
#  汇总计算                                                                     #
# --------------------------------------------------------------------------- #
def day_key(ts):
    return time.strftime("%Y-%m-%d", time.localtime(ts))


def month_key(ts):
    return time.strftime("%Y-%m", time.localtime(ts))


def today_key():
    return time.strftime("%Y-%m-%d", time.localtime())


def month_now():
    return time.strftime("%Y-%m", time.localtime())


def build_series(records):
    """按天出 请求数 / 输入 / 输出 / 缓存 数组，最近 SERIES_DAYS 天。"""
    days = []
    for d in range(SERIES_DAYS - 1, -1, -1):
        days.append(time.strftime("%Y-%m-%d", time.localtime(int(time.time()) - d * 86400)))
    req = [0] * SERIES_DAYS
    inc = [0] * SERIES_DAYS
    outc = [0] * SERIES_DAYS
    cachec = [0] * SERIES_DAYS
    idx = {k: i for i, k in enumerate(days)}
    for r in records:
        k = day_key(r["ts"])
        if k in idx:
            i = idx[k]
            req[i] += 1
            p = r.get("prompt_tokens", 0)
            c = r.get("cache_hit_tokens", 0)
            inc[i] += max(0, p - c)
            cachec[i] += c
            outc[i] += r.get("completion_tokens", 0)
    return req, inc, outc, cachec


def model_table(records):
    """按模型聚合：请求数 / tokens 总量 / 输出 / 缓存占比% / 费用。"""
    agg = {}
    for r in records:
        m = r.get("model", "?")
        a = agg.setdefault(m, dict(req=0, tok=0, outc=0, cache=0, cost=0.0))
        p = r.get("prompt_tokens", 0)
        c = r.get("cache_hit_tokens", 0)
        a["req"] += 1
        a["tok"] += p + r.get("completion_tokens", 0)   # 总量≈输入+输出
        a["outc"] += r.get("completion_tokens", 0)
        a["cache"] += c
        a["cost"] += record_cost_usd(r)
    rows = []
    for m, a in agg.items():
        cache_pct = (100.0 * a["cache"] / a["tok"]) if a["tok"] else 0.0
        rows.append((m, a["req"], a["tok"], a["outc"], cache_pct, a["cost"]))
    rows.sort(key=lambda x: -x[5])
    return rows


def dashboard_text():
    """组装板端可解析的扁平行式文本。"""
    db = load_db()
    records = db["records"]

    # 余额（真实，失败则占位）
    bal_cny, bal_usd, avail = "N/A", "N/A", 0
    if requests is not None and API_KEY:
        try:
            resp = requests.get(DEEPSEEK_URL, headers={
                "Authorization": f"Bearer {API_KEY}", "Accept": "application/json"}, timeout=6)
            if resp.status_code == 200:
                data = resp.json()
                avail = 1 if data.get("is_available") else 0
                for b in data.get("balance_infos", []):
                    v = f"{b.get('total_balance', 0):.4f}"
                    if b.get("currency") == "CNY":
                        bal_cny = v
                    elif b.get("currency") == "USD":
                        bal_usd = v
        except Exception as exc:  # noqa: BLE001
            print(f"[proxy] balance fetch error: {exc}")

    now = int(time.time())
    t_recs = [r for r in records if day_key(r["ts"]) == today_key()]
    m_recs = [r for r in records if month_key(r["ts"]) == month_now()]

    today_cost = sum(record_cost_usd(r) for r in t_recs)
    month_cost = sum(record_cost_usd(r) for r in m_recs)
    req_today = len(t_recs)
    tok_today = sum(r.get("prompt_tokens", 0) + r.get("completion_tokens", 0) for r in t_recs)
    cache_in_today = sum(r.get("cache_hit_tokens", 0) for r in t_recs)
    cache_hit = (100.0 * cache_in_today / sum(r.get("prompt_tokens", 0) + 0.0001 for r in t_recs)) if t_recs else 0.0
    peak = 1 if is_peak(now) else 0
    update = time.strftime("%H:%M", time.localtime())

    req_s, in_s, out_s, cache_s = build_series(records)
    rows = model_table(records)

    def fnum(v):
        return f"{v:.4f}"

    lines = [
        f"OK=1",
        f"BAL_CNY={bal_cny}",
        f"BAL_USD={bal_usd}",
        f"AVAIL={avail}",
        f"TODAY_COST_CNY={fnum(today_cost * RATES_CNY_PER_USD)}",
        f"TODAY_COST_USD={fnum(today_cost)}",
        f"MONTH_COST_CNY={fnum(month_cost * RATES_CNY_PER_USD)}",
        f"REQ_TODAY={req_today}",
        f"TOK_TODAY={tok_today}",
        f"CACHE_HIT={cache_hit:.1f}",
        f"PEAK={peak}",
        f"UPDATE={update}",
        f"REQ_SERIES={','.join(str(x) for x in req_s)}",
        f"IN_SERIES={','.join(str(x) for x in in_s)}",
        f"OUT_SERIES={','.join(str(x) for x in out_s)}",
        f"CACHE_SERIES={','.join(str(x) for x in cache_s)}",
        "MODELS=" + ";".join(
            f"{m}:{r}:{t}:{o}:{cp:.1f}:{fnum(c)}" for (m, r, t, o, cp, c) in rows
        ),
    ]
    return "\r\n".join(lines) + "\r\n"


# --------------------------------------------------------------------------- #
#  HTTP 服务                                                                    #
# --------------------------------------------------------------------------- #
class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):  # 静默 access log
        pass

    def _send(self, code, body, ctype="text/plain"):
        data = body.encode("utf-8") if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    def _cors_head(self):
        return ("Content-Type") in ("text/plain",)  # noqa

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        if self.path.startswith("/api/dashboard"):
            self._send(200, dashboard_text())
        elif self.path in ("/", "/index.html"):
            html = (
                "<html><body style='font-family:sans-serif'>"
                "<h3>DeepSeek proxy</h3>"
                f"<p>API key loaded: <b>{'yes' if API_KEY else 'no'}</b> &nbsp; "
                f"records: <b>{len(load_db()['records'])}</b> &nbsp; "
                f">> use <code>GET /api/dashboard</code></p></body></html>"
            )
            self._send(200, html, "text/html")
        else:
            self._send(404, "not found\n")

    def do_POST(self):
        if self.path == "/log":
            try:
                n = int(self.headers.get("Content-Length", 0))
                body = json.loads(self.rfile.read(n).decode("utf-8") or "{}")
                rec = dict(ts=int(time.time()))
                rec.update(body)
                db = load_db()
                db["records"].append(rec)
                save_db(db)
                self._send(200, f"ok {rec}\n")
            except Exception as exc:  # noqa: BLE001
                self._send(400, f"err {exc}\n")
        else:
            self._send(404, "not found\n")


def local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except OSError:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip


def main():
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="0.0.0.0")
    p.add_argument("--port", type=int, default=8000)
    p.add_argument("--seed-demo", action="store_true", help="填演示数据首屏")
    args = p.parse_args()

    if args.seed_demo:
        seed_demo(load_db())
    if not API_KEY:
        print("[proxy] WARN: DEEPSEEK_API_KEY 未设置，余额将显示 N/A")

    srv = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"[proxy] listening http://{args.host}:{args.port}")
    print(f"[proxy] 板端填: AIDASH_PROXY_IP = \"{local_ip()}\"  (端口 {args.port})")
    if requests is None and API_KEY:
        print("[proxy] WARN: 未安装 requests，余额查询不可用（pip install requests）")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\n[proxy] stopped")


if __name__ == "__main__":
    main()
