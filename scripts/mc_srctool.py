#!/usr/bin/env python3
"""
Comprehensive Minecraft data fetcher.

Combines the functionality of scripts/fetchsrc.py (assets / jar resources) and
scripts/mc_datagen.py (vanilla data generator reports), for one specific version.

Layout written into --output:

    <out>/
      client.jar        client jar (resources mode, unless --no-keep-jar)
      assets/           jar assets + asset-index objects, "assets/" prefix stripped
      data/             data/ tree shipped inside the client jar (vanilla data pack)
      pack.mcmeta       pack.mcmeta from the client jar (if present)
      reports/          datagen reports (blocks.json, registries.json, ...)   [data mode]
      datapack/         datagen --server output (generated data pack contents) [data mode]
      srctool.json      summary of what was fetched

Examples
    scripts/mc_srctool.py --version 1.21.9 --all --output ./mcdata/1.21.9
    scripts/mc_srctool.py -v 1.21.9 --resources --output ./out
    scripts/mc_srctool.py -v 1.21.9 --data --no-datapack --output ./out
    scripts/mc_srctool.py -v 1.21.9 --resources --assets-ext png,json,mcmeta,ogg
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
import urllib.request
import zipfile

MANIFEST_URL = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"
RESOURCE_CDN = "https://resources.download.minecraft.net/%s/%s"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE = os.path.join(ROOT, ".cache", "srctool")
UA = {"User-Agent": "openminecraft-srctool/1.0"}


# --------------------------------------------------------------------------- #
# helpers
# --------------------------------------------------------------------------- #
def log(msg):
    print("[srctool] " + str(msg), flush=True)


def die(msg, code=1):
    print("[srctool] error: " + str(msg), file=sys.stderr, flush=True)
    sys.exit(code)


def download(url, dest=None, retries=3, timeout=120):
    last = None
    for attempt in range(1, retries + 1):
        try:
            req = urllib.request.Request(url, headers=UA)
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                data = resp.read()
            if dest:
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                with open(dest, "wb") as f:
                    f.write(data)
            return data
        except Exception as exc:  # noqa: BLE001 - network errors are varied
            last = exc
            if attempt < retries:
                time.sleep(0.5 * attempt)
    raise RuntimeError("download failed: %s (%s)" % (url, last))


def sha1_of(path):
    with open(path, "rb") as f:
        return hashlib.sha1(f.read()).hexdigest()


def write_if_needed(path, data, force):
    if os.path.exists(path) and not force:
        return False
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)
    return True


# --------------------------------------------------------------------------- #
# version metadata
# --------------------------------------------------------------------------- #
def load_manifest():
    os.makedirs(os.path.join(CACHE, "meta"), exist_ok=True)
    path = os.path.join(CACHE, "meta", "version_manifest_v2.json")
    if not os.path.exists(path) or (time.time() - os.path.getmtime(path)) > 6 * 3600:
        log("fetching version manifest ...")
        download(MANIFEST_URL, path)
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def version_meta(manifest, version_id):
    entry = None
    for v in manifest["versions"]:
        if v["id"] == version_id:
            entry = v
            break
    if entry is None:
        die("version '%s' not found in the manifest" % version_id)

    path = os.path.join(CACHE, "meta", version_id + ".json")
    if not os.path.exists(path):
        log("fetching metadata for %s ..." % version_id)
        download(entry["url"], path)
    with open(path, "r", encoding="utf-8") as f:
        meta = json.load(f)
    meta["_type"] = entry.get("type", "?")
    return meta


def fetch_distribution(meta, version_id, kind):
    info = (meta.get("downloads") or {}).get(kind)
    if not info:
        die("version %s has no '%s' download" % (version_id, kind))

    dest = os.path.join(CACHE, "jars", version_id, kind + ".jar")
    if os.path.exists(dest) and sha1_of(dest) == info["sha1"]:
        log("using cached %s jar" % kind)
        return dest

    log("downloading %s jar (%.1f MiB) ..." % (kind, info["size"] / 1048576.0))
    download(info["url"], dest)

    digest = sha1_of(dest)
    if digest != info["sha1"]:
        os.remove(dest)
        die("sha1 mismatch for %s (expected %s, got %s)" % (dest, info["sha1"], digest))
    log("sha1 ok: " + digest)
    return dest


# --------------------------------------------------------------------------- #
# resources: client jar + asset index
# --------------------------------------------------------------------------- #
def extract_jar_resources(jar, out, force):
    counts = {"assets": 0, "data": 0, "other": 0}

    with zipfile.ZipFile(jar) as zf:
        for info in zf.infolist():
            if info.is_dir():
                continue
            name = info.filename

            if name.startswith("assets/"):
                rel = name[len("assets/"):]
                target = os.path.join(out, "assets", rel)
                counts["assets"] += 1
            elif name.startswith("data/"):
                target = os.path.join(out, "data", name[len("data/"):])
                counts["data"] += 1
            elif name in ("pack.mcmeta", "pack.png"):
                target = os.path.join(out, name)
                counts["other"] += 1
            else:
                continue

            write_if_needed(target, zf.open(name).read(), force)

    log("jar: %d assets, %d data files, %d misc" % (counts["assets"], counts["data"], counts["other"]))
    return counts


def fetch_asset_objects(meta, out, exts, jobs, force):
    index = meta.get("assetIndex")
    if not index:
        log("warning: version has no asset index, skipping asset objects")
        return 0

    index_path = os.path.join(CACHE, "meta", meta.get("id", "?") + "-assets.json")
    if not os.path.exists(index_path):
        log("fetching asset index ...")
        download(index["url"], index_path)
    with open(index_path, "r", encoding="utf-8") as f:
        objects = json.load(f)["objects"]

    def wanted(name):
        if not exts:
            return True
        return any(name.endswith("." + e) for e in exts)

    targets = [(name, obj) for name, obj in objects.items() if wanted(name)]
    log("asset objects: %d selected of %d" % (len(targets), len(objects)))

    fetched = 0

    def worker(item):
        name, obj = item
        target = os.path.join(out, "assets", name)
        if os.path.exists(target) and not force:
            return 0
        h = obj["hash"]
        data = download(RESOURCE_CDN % (h[:2], h))
        if hashlib.sha1(data).hexdigest() != h:
            raise RuntimeError("asset hash mismatch: " + name)
        write_if_needed(target, data, True)
        return 1

    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(worker, t): t[0] for t in targets}
        done = 0
        for fut in concurrent.futures.as_completed(futures):
            done += 1
            try:
                fetched += fut.result()
            except Exception as exc:  # noqa: BLE001
                log("failed asset %s: %s" % (futures[fut], exc))
            if done % 200 == 0 or done == len(futures):
                log("assets: %d/%d processed" % (done, len(futures)))

    log("assets: %d file(s) written" % fetched)
    return fetched


# --------------------------------------------------------------------------- #
# data: vanilla data generator
# --------------------------------------------------------------------------- #
def java_major():
    try:
        out = subprocess.run(["java", "-version"], capture_output=True, text=True).stderr
    except FileNotFoundError:
        die("java not found in PATH (needed for --data)")
    for token in out.replace('"', " ").split():
        if token and token[0].isdigit():
            parts = token.split(".")
            if parts[0] == "1" and len(parts) > 1:
                return int(parts[1])
            return int(parts[0])
    return 0


def is_bundler(jar):
    try:
        with zipfile.ZipFile(jar) as zf:
            return "net/minecraft/bundler/Main.class" in zf.namelist()
    except (zipfile.BadZipFile, OSError):
        return False


def run_datagen(meta, version_id, out, with_datapack, force):
    required = (meta.get("javaVersion") or {}).get("majorVersion")
    major = java_major()
    if required and major < required:
        die("version %s needs java %d, active java is %d" % (version_id, required, major))
    log("java requirement: %s (active: %d)" % (required, major))

    jar = fetch_distribution(meta, version_id, "server")
    workdir = os.path.join(CACHE, "work", version_id)
    generated = os.path.join(workdir, "generated")

    for d in (generated,):
        if os.path.isdir(d) and force:
            shutil.rmtree(d, ignore_errors=True)
    os.makedirs(workdir, exist_ok=True)

    args = ["--reports", "--output", generated]
    if with_datapack:
        args.insert(0, "--server")

    if is_bundler(jar):
        cmd = ["java", "-DbundlerMainClass=net.minecraft.data.Main", "-jar", jar] + args
    else:
        cmd = ["java", "-cp", jar, "net.minecraft.data.Main"] + args

    log("running: " + " ".join(cmd))
    result = subprocess.run(cmd, cwd=workdir)
    if result.returncode != 0 or not os.path.isdir(generated):
        die("data generator failed (exit code %d)" % result.returncode)

    stats = {}
    for sub, target in (("reports", "reports"), ("data", "datapack")):
        src = os.path.join(generated, sub)
        if not os.path.isdir(src):
            continue
        dst = os.path.join(out, target)
        count = 0
        for base, _dirs, files in os.walk(src):
            for name in files:
                full = os.path.join(base, name)
                rel = os.path.relpath(full, src)
                if write_if_needed(os.path.join(dst, rel), open(full, "rb").read(), force):
                    count += 1
        stats[target] = count
        log("%s: %d file(s)" % (target, count))

    if not with_datapack:
        log("datapack generation disabled (--no-datapack)")

    return stats


# --------------------------------------------------------------------------- #
# main
# --------------------------------------------------------------------------- #
def main():
    ap = argparse.ArgumentParser(description="Fetch Minecraft assets and datagen output for one version.")
    ap.add_argument("-v", "--version", required=True, help="version id, e.g. 1.21.9 or 26.4-snapshot-4")
    ap.add_argument("-o", "--output", required=True, help="output directory")
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--all", action="store_true", help="resources + data (default)")
    mode.add_argument("--resources", action="store_true", help="client jar assets and jar-internal data")
    mode.add_argument("--data", action="store_true", help="datagen reports and generated data pack")
    ap.add_argument("--assets-ext", default="png,json,mcmeta",
                    help="asset index extensions to fetch (default: png,json,mcmeta; empty means all)")
    ap.add_argument("--all-assets", action="store_true", help="fetch every asset object (includes sounds)")
    ap.add_argument("--jobs", type=int, default=16, help="parallel asset downloads (default: 16)")
    ap.add_argument("--no-keep-jar", action="store_true", help="do not copy the client jar into the output")
    ap.add_argument("--no-datapack", action="store_true", help="datagen: reports only, skip the generated data pack")
    ap.add_argument("--force", action="store_true", help="overwrite files that already exist")
    args = ap.parse_args()

    do_resources = args.all or args.resources or not (args.resources or args.data)
    do_data = args.all or args.data or not (args.resources or args.data)

    out = os.path.abspath(args.output)
    os.makedirs(out, exist_ok=True)

    manifest = load_manifest()
    meta = version_meta(manifest, args.version)
    log("version: %s (%s)" % (args.version, meta.get("_type", "?")))

    summary = {
        "version": args.version,
        "type": meta.get("_type"),
        "fetchedAt": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "modes": [],
    }

    if do_resources:
        summary["modes"].append("resources")
        log("--- resources ---")

        client = fetch_distribution(meta, args.version, "client")
        counts = extract_jar_resources(client, out, args.force)

        exts = [] if args.all_assets else [e.strip() for e in args.assets_ext.split(",") if e.strip()]
        if exts:
            log("asset extensions: " + ", ".join(exts))
        assets = fetch_asset_objects(meta, out, exts, args.jobs, args.force)

        if not args.no_keep_jar:
            dest = os.path.join(out, "client.jar")
            if not os.path.exists(dest) or args.force:
                shutil.copyfile(client, dest)
                log("copied client jar -> %s" % dest)

        summary["resources"] = {"jarAssets": counts["assets"], "jarData": counts["data"],
                                 "assetObjects": assets}

    if do_data:
        summary["modes"].append("data")
        log("--- data ---")
        stats = run_datagen(meta, args.version, out, not args.no_datapack, args.force)
        summary["data"] = stats

    with open(os.path.join(out, "srctool.json"), "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)

    log("done: %s -> %s" % (args.version, out))


if __name__ == "__main__":
    main()
