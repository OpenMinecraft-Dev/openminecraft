from urllib.request import urlopen
import json
import zipfile
from io import BytesIO
import os
import threading
import time

BASEPATH = "assets"

def savefile(pp, content):
    file_path = os.path.join(BASEPATH, pp)
    os.makedirs(os.path.dirname(file_path), exist_ok=True)
    with open(file_path, 'wb') as f:
        f.write(content)

def ff(k, v):
    while True:
        try:
            savefile(k, urlopen(f"https://resources.download.minecraft.net/{v['hash'][:2]}/{v['hash']}").read())
            print(f"Downloaded {k}")
            break
        except BaseException as e:
            pass

metadata = json.loads(str(urlopen("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json").read(), encoding='utf-8'))
for k in metadata["versions"]:
    if k["id"] == metadata["latest"]["release"]:
        print(f"Fetching version metadata for {k['id']}")
        vermeta = json.loads(str(urlopen(k["url"]).read(), encoding='utf-8'))
        print("Downloading client jar")
        with BytesIO(urlopen(vermeta["downloads"]["client"]["url"]).read()) as bio:
            with zipfile.ZipFile(bio, 'r') as zf:
                for n in zf.infolist():
                    if n.filename.startswith("assets/") and not n.is_dir():
                        print(f"Saving {n.filename}")
                        savefile(n.filename[7:], zf.open(n.filename).read())
        print(f"Fetching assets data")
        assets = json.loads(str(urlopen(vermeta["assetIndex"]["url"]).read(), encoding='utf-8'))
        thrs = []
        for (k, v) in assets["objects"].items():
            t = threading.Thread(target=ff, args=(k,v))
            thrs.append(t)
            t.start()
        
        for tt in thrs:
            tt.join()
        break
