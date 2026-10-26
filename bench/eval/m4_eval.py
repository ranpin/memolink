import base64, io, json, re, time, requests
from PIL import Image
FR = "/home/vision/chenrunbin/vlm/q9_frames/"
# (file, category, gt_fields, gt_desc, license, artist)
MANIFEST = [
 ("sign_level2a.jpg","text-sign",["2A"],"Sign denoting Level 2A at Huntington parking garage","CC BY-SA 4.0","Ser Amantio di Nicolao"),
 ("sign_stairb_level3.jpg","text-sign",["Stair B","Level 3"],"RTD Westminster Station garage, Stair B Level 3 sign","CC BY-SA 4.0","Xnatedawgx"),
 ("speed20.jpg","text-sign",["20"],"Speed Limit 20 sign, Commercial Drive, Southbridge MA","CC0","Peter Cooper Jr."),
 ("speed30nl.jpg","text-sign",["30"],"A 30km/h speed limit sign in Hengelo, Overijssel","CC BY 4.0","Mart de Groot"),
 ("sign_staira_lv2.jpg","text-sign",["Stair A","Level 2"],"Sign for Stair A, Level 2 of Huntington parking garage","CC BY-SA 4.0","Ser Amantio di Nicolao"),
 ("floor7.jpg","text-sign",["7"],"Sign identifying the seventh floor of Venetian parking garage","CC BY-SA 4.0","Eric Polk"),
 ("neon_boulevard.jpg","hard-night",["Boulevard"],"Neon sign of the Boulevard Hotel, Ocean Drive Miami Beach","Public domain","Radomianin"),
 ("faded_ponyexpress.jpg","hard-faded",[],"A worn sign details history of Simpson Springs station","Public domain","BLMUtah"),
 ("faded_welaka.jpg","hard-faded",[],"Worn out sign at Welaka State Forest HQ entrance","CC BY-SA 4.0","DanTD"),
 ("floor_indicator.jpg","text-sign",[],"Floor indicator in Dover Elevator, Port Charlotte HS C building","CC BY-SA 3.0","PCHS-NJROTC"),
 ("dash_parkingbrake.jpg","scene",[],"Amber parking brake warning light glows on car dashboard","CC BY 2.0","Shixart1985"),
 ("seat_rearfacing.jpg","scene",[],"Rear-facing infant car seat","CC BY-SA 3.0","Pieter Kuiper"),
 ("parkingspace_cc0.jpg","scene",[],"Parking space at I-10 rest area, Suwannee County FL","CC0","DanTD"),
 ("dash_vfd.jpg","scene",[],"Digital dashboard cluster of Mercury Grand Marquis, three VFD displays","CC BY-SA 3.0","Ruben de Rijcke"),
]
V4=("这是座舱摄像头拍摄的画面。请用一句完整的陈述句，记录画面中车辆或乘员的持久状态或固定配置"
    "（例如车内安装或放置了什么设备物品、车辆停放的楼层与车位号、仪表的警告状态）。"
    "不要描述瞬时的动作、人物姿态或场景氛围。要求：\n"
    "① 只陈述画面中确实存在的内容，不要推测画面之外的信息；\n"
    "② 画面中若有文字或数字，必须逐字准确读出并写进句子里；\n"
    "③ 看不清的部分直接说「看不清」，不要猜。")
def norm(s): return re.sub(r"[\s，。、；：""''（）()\-_·]+","",s).lower()
def cap(path, le=640):
    im=Image.open(FR+path).convert("RGB"); w,h=im.size; s=le/max(w,h)
    if s<1: im=im.resize((max(1,int(w*s)),max(1,int(h*s))), Image.LANCZOS)
    b=io.BytesIO(); im.save(b,format="JPEG",quality=92)
    p={"model":"qwen3vl","messages":[{"role":"user","content":[
        {"type":"text","text":V4},
        {"type":"image_url","image_url":{"url":"data:image/jpeg;base64,"+base64.b64encode(b.getvalue()).decode()}}]}],
        "temperature":0.0,"max_tokens":256}
    t=time.time(); r=requests.post("http://127.0.0.1:8098/v1/chat/completions",json=p,timeout=300).json()
    return r["choices"][0]["message"]["content"], time.time()-t, list(im.size), r.get("usage",{}).get("prompt_tokens")

results=[]; tot_fields=0; hit_fields=0
print("%-24s %-11s %-6s %-7s %s"%("file","category","wall","size","GT字段命中"))
for f,cat,gt,desc,lic,art in MANIFEST:
    c,dt,sz,pt = cap(f)
    nc = norm(c)
    hits=[]
    for g in gt:
        ok = norm(g) in nc
        hits.append((g,ok))
        tot_fields+=1; hit_fields+= (1 if ok else 0)
    hs = ",".join("%s:%s"%(g,"HIT" if ok else "MISS") for g,ok in hits) if hits else "(无精确GT)"
    print("%-24s %-11s %-6s %-7s %s"%(f,cat,"%.1fs"%dt,"%dx%d"%tuple(sz),hs))
    print("   caption: %s"%c)
    print("   GTdesc : %s | %s | %s"%(desc,lic,art))
    results.append({"file":f,"cat":cat,"gt":gt,"hits":hits,"caption":c,"wall":round(dt,1),"size":sz,"desc":desc,"lic":lic,"artist":art})
json.dump(results, open("/home/vision/chenrunbin/vlm/m4_results.json","w"), ensure_ascii=False, indent=1)
print("\n=== M4b 关键字段：命中 %d/%d ==="%(hit_fields,tot_fields))
